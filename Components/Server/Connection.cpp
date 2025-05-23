#include "Server.hpp"

/**************************************************************/
/*                         Functions                          */
/**************************************************************/

void Server::noBlockingFd()
{
    if (fcntl(this->serverSocket, F_SETFL, O_NONBLOCK) == -1)
        customException("Error : fcntl failed");
}

void Server::socketOptions()
{
    int reuse = 1;
    if (setsockopt(this->serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuse,
                   sizeof(reuse)) == -1)
        customException("Error: setsockopt SO_REUSEADDR failed");
}

void Server::bindServer()
{
    if (bind(this->serverSocket, (const sockaddr *)&(this->serverAddr),
             this->socketLen) == -1)
        customException("Error : bind failed");
}

void Server::listenServer()
{
    if (listen(this->serverSocket, SOMAXCONN) == -1)
        customException("Error : listen failed");
}

void Server::clientDisconnected(int clientFd)
{
    close(clientFd);
    std::cout << Utils::getTime() << " " << this->users.at(clientFd).getNickName() << " has left the server." << std::endl;
    if (this->buffring.find(clientFd) != this->buffring.end())
        this->buffring.erase(this->buffring.find(clientFd));
    if (this->nicks.find(this->users.at(clientFd).getNickName()) != this->nicks.end())
        this->nicks.erase(this->nicks.find(this->users.at(clientFd).getNickName()));
    if (this->users.find(clientFd) != this->users.end())
    {
        this->users[clientFd].leaveAllChannels(&(this->responses));
        this->users.erase(clientFd);
    }

    for (std::vector<struct pollfd>::iterator it = this->pfds.begin(); it != this->pfds.end(); it++)
    {
        if (it->fd == clientFd)
        {
            this->pfds.erase(it);
            break;
        }
    }
}

void Server::requests(int indexClient)
{
    if (this->pfds[indexClient].revents & (POLLHUP | POLLERR))
        clientDisconnected(this->pfds[indexClient].fd);
    else if (this->pfds[indexClient].revents & POLLIN)
    {
        char buffer[1000];
        std::memset(buffer, 0, sizeof(buffer));
        int r = recv(this->pfds[indexClient].fd, buffer, sizeof(buffer) - 1, 0);
        if (r > 512)
            std::cout << Utils::getTime() << "buffer limite exceeded by :" << this->users.at(this->pfds[indexClient].fd).getHostName() << std::endl;
        else if (r <= 0)
            clientDisconnected(this->pfds[indexClient].fd);
        else
        {
            std::string command = "";
            if (strlen(buffer) > 0 && buffer[strlen(buffer) - 1] != '\n')
                this->buffring[this->pfds[indexClient].fd] += buffer;
            else
            {
                
                this->buffring[this->pfds[indexClient].fd] += buffer;
                std::string command = this->buffring[this->pfds[indexClient].fd];
                std::stringstream data(command);
                this->buffring[this->pfds[indexClient].fd].clear();
                while (std::getline(data, command, '\n'))
                {
                    if (command.find('\r') != std::string::npos)
                        command = command.substr(0, command.find('\r'));
                    runCommand(this->pfds[indexClient].fd, command);
                }
                data.clear();
            }
            
        }
    }
}

void Server::acceptAndDecline()
{
    struct sockaddr_in clientAddr;
    unsigned int csocketLen = sizeof(clientAddr);
    int client_fd = accept(this->serverSocket, (sockaddr *)&clientAddr, (socklen_t *)&csocketLen);
    if (client_fd == -1)
    {
        std::cerr << RED << "Error : accept failed (max_fd reached)" << DEFAULT << std::endl;
        return;
    }
    std::cout << Utils::getTime() << " " << inet_ntoa(clientAddr.sin_addr)
              << " has joined the server." << std::endl;
    this->pfds.push_back((struct pollfd){.fd = client_fd, .events = POLLIN, .revents = 0});
    this->users.insert(std::make_pair(client_fd, User(client_fd)));
    this->users[client_fd].setHostName(inet_ntoa(clientAddr.sin_addr));
}

void Server::multipleClients()
{
    int timeout = 10;
    for (;;)
    {
        int numfds = poll(&(this->pfds[0]), this->pfds.size(), timeout);
        if (numfds == -1)
            customException("Error : poll failed");
        if (this->pfds[0].revents & POLLIN)
            acceptAndDecline();
        else if (numfds > 0)
            for (size_t i = 0; i < this->pfds.size(); i++)
                requests(i);
        if (!this->responses.empty())
        {
            timeout = 0;
            std::pair<int, t_message> response = this->responses.front();
            this->responses.pop();
            if (this->channels[response.second.channelName].validateResponse(response.first, response.second))
                send(response.first, response.second.message.c_str(), response.second.message.size(), 0);
        }
        else
            timeout = 10;
    }
}