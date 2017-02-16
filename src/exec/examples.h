/** TEMPORARY EXAMPLES FOR TESTING */

#include "../core/utils/log.h"

#include "../core/module/SingleModuleFactory.hpp"

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
    
    TestModuleOne(AllPix *apx): Module(apx) {}
    
    std::string getName(){
        return TestModuleOne::name;
    }
    
    void init(Configuration conf){
        conf.setDefault("name", "standard_one_name");
        conf.setDefault("message", "standard_message");
        LOG(DEBUG) << "(1) init and add to run queue for module " << conf.get<std::string>("name");
        
        conf_ = conf;
        getModuleManager()->addToRunQueue(this);
    }
    
    void run(){
        LOG(DEBUG) << "(1) running first module";
        TestMessageTwo test;
        test.setText(conf_.get<std::string>("message"));
        
        getMessenger()->dispatchMessage(test, "random_name");
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
    
    TestModuleTwo(AllPix *apx): Module(apx) {}
    
    void init(Configuration conf){
        conf_ = conf;
        conf_.setDefault("name", "standard_two_name");
        LOG(DEBUG) << "(2) init registering listeners for module " << conf_.get<std::string>("name");
        
        getMessenger()->registerListener(this, &TestModuleTwo::receive);
    }
    
    void receive(TestMessageTwo msg){
        messages_.push_back(msg.getText());
        LOG(DEBUG) << "(2) received a message: " << msg.getText();
        LOG(DEBUG) << "    add to run queue ";
        
        getModuleManager()->addToRunQueue(this);
    }
    
    std::string getName(){
        return TestModuleTwo::name;
    }
    
    void run(){
        IFLOG(DEBUG) {
            std::string str = "";
            for(auto &msg : messages_) str += msg+", ";
            if(!str.empty()) str = str.substr(0, str.size()-2);
            LOG(DEBUG) << " (2) running second module with message " << str;
        }
    }
    
    void finalize(){
        LOG(DEBUG) << "(2) finished";
    }
private:
    std::vector<std::string> messages_;
    Configuration conf_;
};

const std::string TestModuleTwo::name = "test2";

std::unique_ptr<ModuleFactory> generator(std::string str){
    if(str == TestModuleOne::name) return std::make_unique<SingleModuleFactory<TestModuleOne>>();
    if(str == TestModuleTwo::name) return std::make_unique<SingleModuleFactory<TestModuleTwo>>();
    return nullptr;
}
