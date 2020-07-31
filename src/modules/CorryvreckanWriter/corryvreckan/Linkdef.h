/**
 * @file
 * @brief Linkdef for ROOT Class generation
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
