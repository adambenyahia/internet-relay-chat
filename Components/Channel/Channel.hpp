#ifndef CHANNEL_HPP
#define CHANNEL_HPP
#include "../Components.hpp"
#include "../User/User.hpp"

class User; 
class Channel{
    private :
        std::string name;
        std::string key;
        std::string topic;
        std::map<int, User*> users;
        std::map<int, User*> operators;
        std::vector<std::string> invited;
        int memberCount;
        int limit;
        int mode;

    public :
    
        Channel();
        Channel(std::string name, User *user);
        Channel(std::string name);
        Channel(const Channel &channel);
        ~Channel();
        

        std::string getName() const;
        void setName(std::string name);
        std::string getKey() const;
        void setKey(std::string key);
        std::string getTopic() const;
        void setTopic(std::string topic);

        std::string getUserNickByFd(int fd) const;
        const std::map<int, User*>& getUsers() const;
        const std::map<int, User*>& getOperators() const;
        int getLimit() const;
        void setLimit(int limit);
        int getMemberCount() const;
        void setMemberCount(int memberCount);
        
        int getMode() const;
        void setMode(int mode);

        std::string getModeString() const;


        bool isOperator(int clientFd);
        void unsetMode(int mode);
        void addUser(User *user);
        void addInvited(std::string nickName);
        void removeInvited(std::string nickName);
        bool isInvited(std::string nickName);
        void addOperator(User *user);
        void removeUser(int clientFd);
        void broadcast(User *sender, std::string message, std::queue<std::pair<int, t_message> > *queue,  bool all = true);
        void sendNames(int clientFd, std::string nickName);
        void changeOpMode(User *user, int mode);
        bool validateResponse(int recieverFd, t_message response);
        enum MODES{
            NO_MODE = 0,
            INVITE_ONLY = 1,
            TOPIC_PROTECTED = 2,
            MODERATED = 4,
            LIMIT = 8,
            KEY = 16,
        };
        void listUsers();
};
#endif //CHANNEL_HPP