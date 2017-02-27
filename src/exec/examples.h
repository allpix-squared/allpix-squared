/** TEMPORARY EXAMPLES FOR TESTING */

#include "../core/utils/log.h"

#include "../core/module/UniqueModuleFactory.hpp"
#include "../core/module/DetectorModuleFactory.hpp"

#include <memory>

using namespace allpix;

class TestMessageOne : public Message{
public:
    void setText(std::string text){
        text_.clear();
        text_ = "[1] ";
        text_ += text;
    }
    std::string getText(){
        return text_;
    }
private:
    std::string text_;
};

class TestMessageTwo : public Message{
public:
    void setText(std::string text){
        text_.clear();
        text_ = "[2] ";
        text_ += text;
    }
    std::string getText(){
        return text_;
    }
private:
    std::string text_;
};

class TestModuleOne : public Module{
public:
    static const std::string name;
    
    TestModuleOne(AllPix *apx, ModuleIdentifier id, Configuration config): Module(apx, id) {
        //conf.setDefault("test", "standard_one_name");
        conf_ = config;
        
        conf_.setDefault("message", "standard_message");
        LOG(DEBUG) << "(1) init and add to run queue for module " << conf_.getText("name", "<none>");
        
        // getModuleManager()->addToRunQueue(this);
        // getConfig().get()
        
    }
    
    /*std::string getName(){
        return TestModuleOne::name;
    }*/
    
    void run(){
        LOG(DEBUG) << "(1) running first module";
        TestMessageTwo test;
        test.setText(conf_.get<std::string>("message"));
        
        getMessenger()->dispatchMessage(test);
    }
    
    void finalize(){
        LOG(DEBUG) << "(1) this is the end of module 1";
    }
private:
    Configuration conf_;
};

const std::string TestModuleOne::name = "test1";

class TestModuleTwo : public Module{
public:
    static const std::string name;
    
    TestModuleTwo(AllPix *apx, ModuleIdentifier id, Configuration config): Module(apx, id) {
        conf_ = config;
        conf_.setDefault("test", "standard_two_name");
        LOG(DEBUG) << "(2) init registering listeners for module " << conf_.getText("name", "<none>");
        
        //getMessenger()->registerListener(this, &TestModuleTwo::receive);
        getMessenger()->bindMulti(this, &TestModuleTwo::messages_);
    }
    
    void receive(std::shared_ptr<TestMessageTwo> msg){
        LOG(DEBUG) << "(2) received a message: " << msg->getText();
    }
    
    /*std::string getName(){
        return TestModuleTwo::name;
    }*/
    
    void run(){
        // messages_.push_back(message_->getText());
        
        IFLOG(DEBUG) {            
            std::string str = "";
            for(auto &msg : messages_) str += msg->getText()+", ";
            if(!str.empty()) str = str.substr(0, str.size()-2);
            LOG(DEBUG) << " (2) running second module with message " << str;
        }
    }
    
    void finalize(){
        LOG(DEBUG) << "(2) finished";
    }
private:
    std::vector<std::shared_ptr<TestMessageTwo>> messages_;
    
    //std::vector<std::string> messages_;
    Configuration conf_;
};

const std::string TestModuleTwo::name = "test2";
