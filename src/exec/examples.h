/** TEMPORARY EXAMPLES FOR TESTING */

#include "../core/utils/log.h"

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
    
    void init(Configuration){
        LOG(DEBUG) << "(1) init first module";
        
        getModuleManager()->addToRunQueue(this);
    }
    
    void run(){
        LOG(DEBUG) << "(1) running first module";
        TestMessageTwo test;
        test.setText("hello from module 1");
        
        getMessenger()->dispatchMessage("test", test);
    }
    
    void finalize(){
        LOG(DEBUG) << "(1) this is the end of module 1";
    }
};

const std::string TestModuleOne::name = "test1";

class TestModuleTwo : public Module{
public:
    static const std::string name;
    
    TestModuleTwo(AllPix *apx): Module(apx) {}
    
    void init(Configuration){
        LOG(DEBUG) << "(2) init registering listeners for module 2";
        
        getMessenger()->registerListener(this, &TestModuleTwo::receive, "test");
    }
    
    void receive(TestMessageTwo msg){
        LOG(DEBUG) << "(2) received a message: " << msg.getText();
        LOG(DEBUG) << "    add to run queue ";
        
        getModuleManager()->addToRunQueue(this);
    }
    
    std::string getName(){
        return TestModuleTwo::name;
    }
    
    void run(){
        LOG(DEBUG) << "(2) running";
    }
    
    void finalize(){
        LOG(DEBUG) << "(2) finished";
    }
};

const std::string TestModuleTwo::name = "test2";

Module *generator(std::string str, AllPix *allpix){
    if(str == TestModuleOne::name) return new TestModuleOne(allpix);
    if(str == TestModuleTwo::name) return new TestModuleTwo(allpix);
    return nullptr;
}
