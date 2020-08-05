/**
 * @file
 * @brief Linkdef for ROOT Class generation
 */

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;
#pragma link C++ nestedclasses;

// Missing ROOT objects
#pragma link C++ class ROOT::Math::Cartesian2D < unsigned int> + ;
#pragma link C++ class ROOT::Math::DisplacementVector2D < ROOT::Math::Cartesian2D < unsigned int>,                          \
    ROOT::Math::DefaultCoordinateSystemTag> +                                                                               \
    ;

// AP2 objects
#pragma link C++ class allpix::Object + ;
#pragma link C++ class allpix::MCTrack + ;
#pragma link C++ class allpix::Object::ReferenceWrapper < allpix::MCTrack> + ;
#pragma link C++ class allpix::MCParticle + ;
#pragma link C++ class allpix::Object::ReferenceWrapper < allpix::MCParticle> + ;
#pragma link C++ class allpix::SensorCharge + ;
#pragma link C++ class allpix::PropagatedCharge + ;
#pragma link C++ class allpix::Object::ReferenceWrapper < allpix::PropagatedCharge> + ;
#pragma link C++ class allpix::DepositedCharge + ;
#pragma link C++ class allpix::Object::ReferenceWrapper < allpix::DepositedCharge> + ;
#pragma link C++ class allpix::Pixel + ;
#pragma link C++ class allpix::PixelCharge + ;
#pragma link C++ class allpix::Object::ReferenceWrapper < allpix::PixelCharge> + ;
#pragma link C++ class allpix::PixelHit + ;
#pragma link C++ class allpix::Object::ReferenceWrapper < allpix::PixelHit> + ;
#pragma link C++ class allpix::Pulse + ;

// Vector of Object for internal storage
#pragma link C++ class std::vector < allpix::Object*> + ;
