---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Writing Thread-Safe Code"
weight: 4
---

In Allpix Squared events are processed fully parallel on separate threads which requires some consideration when writing
module code. This section briefly lists the most important aspects to take into account.

## Member Variables

While the `initialize()` and `finalize()` of the module are guaranteed to be called sequentially, the `run()` method will be
called simultaneously from different threads and for different events. Therefore, no module data members must be altered from
within the `run()` function, otherwise these changes will affect other events being processed in parallel on other threads.
Configuration parameters cached as member variables should therefore be set only in the `initialize()` function.

For initialization and finalization of thread-local data members, i.e. structures which have to be configured for each of the
worker threads the module is executed on, the `initializeThread()` and `finalizeThread()` methods are available. They are
called once on each worker thread after the `initialize()` and before the `finalize()` methods, respectively.

## Histograms

Allpix Squared uses ROOT histograms for collecting and storing statistics and other additional information about the
simulation process. ROOT provides the template class `ROOT::TThreadedObject` which allows to use histograms in multithreaded
environments but slightly alters the interface of the histogram objects. Furthermore, there have been significant changes to
the class between minor release version of ROOT and it doesn't scale well with a large number of predefined threads.
Therefore, Allpix Squared provides its own re-implementation of this class, `allpix::ThreadedHistogram` which also restores
the original interface of the histogram classes, i.e. it is possible to instantiate, fill and store histograms the same way
as in a single-threaded environment.

This class can be used as follows:

```cpp
// Declaration of a new histogram of type "TH1D"
Histogram<TH1D> my_histogram;

// Creation of the histogram using the CreateHistogram helper method:
my_histogram = CreateHistogram<TH1D>("name", "title", 100, 0., 100.);

// Filling, setting bin contents and writing the histogram works as before:
my_histogram->Fill(12.);
my_histogram->SetBinContent(15, 23.);
my_histogram->Write();
```

## Declaring a Module Thread-Safe

If a module is thread-safe, i.e. its `run()` function can be called from different threads in parallel without locking, it
can be declared as thread-safe to the framework. In this case the ModuleManager will allow multithreading of calls to this
module.

This declaration is done by placing the following call in the constructor of the module:

```cpp
MyParallelModule::MyParallelModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector)
    : Module(std::move(config), std::move(detector)) {
    // This module is thread-safe and can be called from different threads simultaneously:
    allow_multithreading();
}
```

By adding this statement, the module certifies to work correctly if its `run()` method is executed multiple times in
parallel, for different events. This means in particular that the module will safely handle access to shared (for example
static) variables as described in [Section 10.4](../10_development/04_thread_safe_code.md#member-variables) and that it will
properly assign and bind ROOT histograms to their respective directories in the output ROOT file before the event processing
starts and the `run()` method is called the first time. Access to constant operations in the `GeometryManager`, `Detector`
and `DetectorModel` is always valid between various threads. In addition, sending and receiving messages is thread-safe.

Since multithreading might be disabled by other modules in the chain or by the user via the configuration file or command
line, it might be required to check at runtime of the module if it is currently running in a multithreaded environment. This
can be achieved with the following method:

```cpp
MyParallelModule::run(Event* event) {
    if(multithreadingEnabled()) {
        // This module is currently running in a multithreaded environment
    } else {
        // This module is running in a fully sequential environment
    }
}
```
