/**
 * @file
 * @brief Linkdef for ROOT Class generation
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

// Corryvreckan objects
#pragma link C++ class corryvreckan::Object + ;
#pragma link C++ class corryvreckan::Pixel + ;
#pragma link C++ class corryvreckan::MCParticle + ;
#pragma link C++ class corryvreckan::Event + ;

// Vector of Object for internal storage
#pragma link C++ class std::vector < corryvreckan::Object*> + ;
