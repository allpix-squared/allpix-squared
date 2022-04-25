---
# SPDX-FileCopyrightText: 2022 CERN and the Allpix Squared authors
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
or write access to events. This enables output modules to produce the exact same output file for the same simulation inputs
without sacrificing the benefits of using multithreading for other modules.

Since random number generators are thread-local and shared between events processed on the same thread, their state is stored
internally when being written into the buffer and restored before processing. This ensures that the sequence of pseudo-random
numbers is exactly the same regardless of whether the event was buffered or directly processed.

### Geant4 Modules

The usage of the Geant4 library in Allpix Squared has some constraints because the Geant4 multithreaded run manager expects
to handle parallelization internally which violates the Allpix Squared design. Furthermore, Geant4 does not guarantee results
reproducibility between its multithreaded and sequential run managers. Modules that would like to use the Geant4 library
shall not use the run managers provided by Geant4. Instead, they must use the custom run managers provided by Allpix Squared
as described in [Section 13.1](../13_additional/01_tools.md#geant4-interface).
