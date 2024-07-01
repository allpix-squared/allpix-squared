---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Multithreading"
weight: 10
---

Allpix Squared supports multithreading by running events in parallel. The module manager creates a thread pool with the
configured number of workers or determines them from system parameters if not specified. Each event is represented by an
instance of the `Event` class which encapsulates the data used during this event. The configured number of events are then
submitted to the thread pool and executed by the thread pool's workers.

The thread pool features two independent queues. A FIFO-like unsorted queue for events to be processed, and a second,
priority-ordered queue for buffered events. The former is constantly filled with new events to be processed by the main
thread, while the latter is used to temporarily buffer events which wait to be picked up in the correct sequence by a
`SequentialModule`.

By default modules are assumed to not operate in a thread-safe way and therefore cannot participate in multithreaded
processing of events. Therefore each module must explicitly enable multithreading in its constructor in order to signal its
multithreading capabilities to Allpix Squared. To support multithreading, the module `run()` method should be re-entrant and
any shared member variables should be protected. If multithreading is enabled in the run configuration, the module manager
will check if all the loaded modules support multithreading. In case one or more modules do not support multithreading, a
warning is printed and the feature is disabled. Modules can inform themselves about the decision via the
`multithreadingEnabled()` method.

### Seed Distribution

A stable seed distribution to modules and core components of Allpix Squared is guaranteed in order to be able to provide
reproducibility of simulation results from the same inputs even when the number of workers is different. Each event is seeded
upon its creation by the main thread from a central event seed generator, in increasing sequence of event numbers. The event
provides access to a random engine that can be used by each module in the `run()` method.

To avoid the memory overhead of maintaining random engine objects equal to the number of events, the storage of the engines
is made static and thread-local, and is only provided to the event for temporary usage. This way ensures that the framework
maintains the minimum number of such heavy objects equal to the number of workers used. When a worker starts to execute a new
event, it seeds its local random engine first and passes it to the event object.

### Using Messenger in Parallel

The `Messenger` handles communication in different events concurrently. It supports dispatching and fetching messages via the
`LocalMessenger`. Each event has its own local messenger which stores all messages that was produced in this event. The
`Messenger` owns the global message subscription information and internally forwards the module's requests to dispatch or
fetch messages to the local messenger of the event in a thread-safe manner.

### Running Events in order using SequentialModule

The `SequentialModule` class is made available for modules that require processing of events in the correct order without
disabling multithreading. Inheriting from this class will allow the module to transparently check if the given event is in
the correct sequence and decide whether to execute it immediately or to request buffering in the prioritized buffer queue if
the thread pool if it is out of order.

Using the `SequentialModule` is suitable for I/O modules which read or write to the file system and do not allow random read
or write access to events. This enables output modules to produce the same output file for the same simulation inputs
without sacrificing the benefits of using multithreading for other modules.

Since random number generators are thread-local and shared between events processed on the same thread, their state is stored
internally when being written into the buffer and restored before processing. This ensures that the sequence of pseudo-random
numbers is exactly the same regardless of whether the event was buffered or directly processed.

### Geant4 Modules

The usage of the Geant4 library in Allpix Squared has some constraints because the Geant4 multithreaded run manager expects
to handle parallelization internally which violates the Allpix Squared design. Furthermore, Geant4 does not guarantee results
reproducibility between its multithreaded and sequential run managers. Modules that would like to use the Geant4 library
shall not use the run managers provided by Geant4. Instead, they must use the custom run managers provided by Allpix Squared
as described in [Section 14.1](../14_additional/01_tools.md#geant4-interface).

### Object History, TRefs and PointerWrappers

Allpix Squared uses ROOTs `TRef` objects to store the persistent links of the simulation object history. These references act
similar to C pointers and allow accessing the referenced object directly without additional bookkeeping or lookup of indices.
Furthermore they persist when being written to a ROOT file and read back to memory. ROOT implements this via a central lookup
table that keeps track of the referenced objects and their location in memory as described
[in the ROOT documentation](https://root.cern.ch/doc/master/classTRef.html).

This approach comes with some drawbacks, especially in multithreaded environments. Most importantly the lookup table is a
global object, which means mutexes are required for accessing it. Multiple threads generating or using `TRef` references will
have to share this mutex and will consequently be subject to significant waiting for lock release. Furthermore generating more
and more `TRef` relations over the course of a simulation will increase the size of the central reference table. This table is
initialized with a fixed size, and once the number of `TRef` objects outgrows this pre-allocated space, new memory has to be
acquired, leading to a reallocation of memory for the entire new size of the table. With potentially millions of entries, this
quickly becomes a computationally expensive operation, slowing down the simulation significantly.

Allpix Squared solves these limitations by wrapping the `TRef` objects into a class called `PointerWrapper`. It contains both
a direct, but transitional C pointer and a `TRef` to the referenced object. The latter, however, is only generated when
required, i.e. if the object holding the `PointerWrapper` as well as referenced object are going to be written to file. This
is achieved by first going through all relevant objects, marking them for storage:

```cpp
for(auto& object : objects) {
    object.markForStorage();
}
```

Now, the required history references can be identified and `TRef` objects are generated *only* for relations between two objects
that are both marked for storage:

```cpp
for(auto& object : objects) {
    object.petrifyHistory();
}
```

Objects can now be written to file and will contain the persistent reference to the related object.

This approach solves the above problems. File writing has to be performed single-threaded anyway, so generating `TRef` objects
on the same thread does not lead to additional locking of the central reference table mutex in root. In addition, `TRef` entries
are only generated and stored in the table for objects that require it - all references to objects not to be stored will be
`nullptr` in either case since the target object is not available anymore when reading in the data. Since now the generation of
`TRef` objects and access to the reference table is performed by a single thread and one single event at a time, it is also
possible to reset the ROOT-internal object ID of `TRef` references after the event has been processed. The subsequent event will
reuse the same IDs again, preventing a continuous growth of the reference table and related memory re-allocation issues.

As a consequence, when reading objects back from file in a multithreaded environment, the `TRef` has to be converted back to a C
memory pointer in the reading thread, both to prevent mixing of re-used `TRef` object IDs from different events and to avoid
locking access to the central reference table when looking up the memory location from there. This is performed similarly to the
generation of history relations, and here only relations to valid TRefs are loaded, other relations will hold a `nullptr`:

```cpp
for(auto& object : objects) {
    object.loadHistory();
}
```

For single-threaded applications such as ROOT analysis macros, this step is not necessary and the reference will be lazy-loaded
when accessed, i.e. the `TRef` reference will be converted to a direct raw pointer only when actually used. Since events are
processed sequentially and memory is freed between events, no mixing of IDs occurs.
