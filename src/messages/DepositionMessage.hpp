/**
 * Message for a deposited charge 
 * 
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MESSAGE_DEPOSITION_H
#define ALLPIX_MESSAGE_DEPOSITION_H

#include <memory>
#include <string>

#include <TVector3.h>

#include "../core/messenger/Message.hpp"

namespace allpix{
    // type of the deposits
    class ChargeDeposit{
    public:
        ChargeDeposit(TVector3 position, double energy);
        
        TVector3 getPosition();
        double getEnergy();
    private:
        TVector3 position_;
        double energy_;
    };
    
    // message carrying all the deposits
    class DepositionMessage : public Message {
    public:
        DepositionMessage(): deposits() {}
        std::vector<ChargeDeposit> &getDeposits();
        
    private:
        std::vector<ChargeDeposit> deposits;
    };
}

#endif /* ALLPIX_MESSAGE_DEPOSITION_H */
