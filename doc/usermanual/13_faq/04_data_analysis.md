---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Data Analysis"
weight: 4
---

#### How do I access the history of a particular object?

Many objects can include an internal link to related other objects (for example `getPropagatedCharges` in the `PixelCharge`
object), containing the history of the object (thus the objects that were used to construct the current object). These
referenced objects are stored as special ROOT pointers inside the object, which can only be accessed if the referenced object
is available in memory. In Allpix Squared this requirement can be automatically fulfilled by also binding the history object
of interest in a module. During analysis, the tree holding the referenced object should be loaded and pointing to the same
event entry as the object that requests the reference. If the referenced object can not be loaded, an exception is thrown by
the retrieving method. Please refer to [Section 7.2](../07_objects/02_object_history.md) for more information.

#### How do I access the Monte Carlo truth of a specific PixelHit?

The Monte Carlo truth is part of the history of a PixelHit. This means that the Monte Carlo truth can be retrieved as
described in the question above. Because accessing the Monte Carlo truth of a PixelHit is quite a common task, these
references are stored directly for every new object created. This allows to retain the information without the necessity to
keep the full object history including all intermediate steps in memory. Please refer to
[Section 7.2](../07_objects/02_object_history.md) for more information.

#### How do I find out, which Monte Carlo particles are primary particles and which have been generated in the sensor?

The Monte Carlo truth information is stored per-sensor as MCParticle objects. Each MCParticle stores, among other
information, a reference to its parent. Particles which have entered the sensor from the outside world do not have parent
MCParticles in the respective sensor and are thus primaries.

Using this approach it is possible, to e.g. treat a secondary particle produced in one detector as primary in a following
detector.

Below is some pseudo-code to filter a list of MCParticle objects for primaries based on their parent relationship:

```cpp
// Collect all primary particles of the event:
std::vector<const MCParticle*> primaries;

// Loop over all MCParticles available
for(auto& mc_particle : my_mc_particles) {
    // Check for possible parents:
    if(mc_particle.getParent() != nullptr) {
        // Has a parent, thus was created inside this sensor.
        continue;
    }

    // Has no parent particles in this sensor, add to primary list.
    primaries.push_back(&mc_particle);
}
```

A similar function is used e.g. in the DetectorHistogrammer module to filter primary particles and create position-resolved
graphs. Furthermore, the PixelHit and PixelCharge objects provide two member functions to access Monte Carlo particles, one
which returns all known particles, `getMCParticles()`, and a second function called `getPrimaryMCParticles()` which already
performs the above filtering and only returns primary particle references.

#### How do I access data stored in a file produced with the ROOTObjectWriter from an analysis script?

Allpix Squared uses ROOT trees to directly store the relevant C++ objects as binary data in the file. This retains all
information present during the simulation run, including relations between different objects such as assignment of Monte
Carlo particles. In order to read such a data file in an analysis script, the relevant library as well as its header have to
be loaded.

In ROOT this can be done interactively by loading a data file, the necessary shared library objects and a macro for the
analysis:

```shell
root -l data_file.root
root [1] .L ~/path/to/your/allpix-squared/lib/libAllpixObjects.so
root [2] .L analysisMacro.C+
root [3] readTree(_file0, "detector1")
```

A simple macro for reading DepositedCharges from a file and displaying their position is presented below:

```cpp
#include <TFile.h>
#include <TTree.h>

// FIXME: adapt path to the include file of APSQ installation
#include "/path/to/your/allpix-squared/DepositedCharge.hpp"

// Read data from tree
void readTree(TFile* file, std::string detector) {

    // Read tree of deposited charges:
    TTree* dc_tree = static_cast<TTree*>(file->Get("DepositedCharge"));
    if(!dc_tree) {
        throw std::runtime_error("Could not read tree");
    }

    // Find branch for the detector requested:
    TBranch* dc_branch = dc_tree->FindBranch(detector.c_str());
    if(!dc_branch) {
        throw std::runtime_error("Could not find detector branch");
    }

    // Allocate object vector and link to ROOT branch:
    std::vector<allpix::DepositedCharge*> deposited_charges;
    dc_branch->SetObject(&deposited_charges);

    // Go through the tree event-by-event:
    for(int i = 0; i < dc_tree->GetEntries(); ++i) {
        dc_tree->GetEntry(i);
        // Loop over all deposited charge objects
        for(auto& charge : deposited_charges) {
            std::cout << "Event " << i << ": "
                        << "charge = " << charge->getCharge() << ", "
                        << "position = " << charge->getGlobalPosition()
                        << std::endl;
        }
    }
}
```

A more elaborate example for a data analysis script can be found in the `tools` directory of the repository \[[@ap2-repo]\]
and in [Section 14.3](../14_additional/root_analysis_macros.md). Scripts written in both C++ and in Python are provided.

#### How can I convert data from the ROOTObject format to other formats?

Since the ROOTObject format is the native format of Allpix Squared, the stored data can be read into the framework again. To
convert it to another format, a simple pseudo-simulation setup can be used, which reads in data with one module and stores it
with another.

In order to convert for example from ROOTObjects to the data format used by the Corryvreckan reconstruction framework, the
following configuration could be used:

```ini
[Allpix]
number_of_events = 999999999
detectors_file = "telescope.conf"
random_seed_core = 0

[ROOTObjectReader]
file_name = "input_data_rootobjects.root"

[CorryvreckanWriter]
file_name = "output_data_corryvreckan.root"
reference = "mydetector0"
```


[@ap2-repo]: https://gitlab.cern.ch/allpix-squared/allpix-squared
