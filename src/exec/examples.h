/** TEMPORARY EXAMPLES FOR TESTING */

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
    void start(Messenger *msg){
        TestMessageTwo test;
        test.setText("hello from module 1");
        msg->dispatchMessage("test", test);
    }
    
    std::string getName(){
        return "test1";
    }
    
    void run(){
        std::cout << "(1) running" << std::endl;
    }
    
    void finalize(){
        std::cout << "(1) this is the end" << std::endl;
    }
};

class TestModuleTwo : public Module{
public:
    void start(Messenger *msg){
        msg->registerListener(this, &TestModuleTwo::receive, "test");
    }
    
    void receive(TestMessageTwo msg){
        std::cout << "Module 2 received a message: " << msg.getText() << std::endl;
    }
    
    std::string getName(){
        return "test2";
    }
    
    void run(){
        std::cout << "(2) running" << std::endl;
    }
    
    void finalize(){
        std::cout << "(2) this is the end" << std::endl;
    }
};
