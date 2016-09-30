#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "TCP/TCPAcceptor.h"
#include "Application/ClientQueue.h"
#include "Utilities.h"
#include <vector>
#include "Application/Channel.h"
#include <string>
#include <thread>


using namespace std;

void new_connection(TCPStream *stream, ClientQueue *c);
void new_channel(Channel* newch, User* person1, User* person2);
void connectionManager(ClientQueue* c);


TCPAcceptor* acceptor = NULL;
vector<Channel*> channels;

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("Usage: server <port>\n");
        exit(1);
    }

    ClientQueue cqueue = ClientQueue();
    acceptor = new TCPAcceptor(atoi(argv[1]));
    channels = vector<Channel*>();

    if (acceptor->start() == 0)
    {
        std::thread connectionThread(connectionManager, &cqueue);
        connectionThread.detach();

        for(;;)
        {
            if(cqueue.getSize() > 1){

                User* firstperson = cqueue.getNext();
                User* secondperson = cqueue.getNext();

                Channel *newChannel = new Channel();

                channels.push_back(newChannel);

                new_channel(newChannel, firstperson, secondperson);

            }

        }
    }
    exit(0);
}

void new_channel(Channel *ch, User* person1, User* person2){

    ch->addUser(person1);
    ch->addUser(person2);

    person1->setActiveChannel(ch);
    person2->setActiveChannel(ch);

    send("Found someone! Connecting you to a chat.", person1->getUserStream());
    send("Found someone! Connecting you to a chat.", person2->getUserStream());

}

void new_connection(TCPStream *stream, ClientQueue *cqueue){
    ssize_t len;
    char line[1000];
    User me;

    while ((len = stream->receive(line, sizeof(line))) > 0)
    {
        line[len]=0;
        std::ostringstream oss;
        string rec(line);

        vector<string> splitCommand = split(rec, ' ');
        if(splitCommand[0].compare("CONNECT") == 0) {
            me = User(splitCommand[1], stream,  false);
            cqueue->addUser(&me);

            int queuesize = cqueue->getSize();
            oss << "You are position" << queuesize << "in the queue.\n";
            send(oss.str(), stream);
            oss.flush();

            if(cqueue->getSize() <= 1) {
                oss << "Currently no one else is online.\n Sorry :(.\n We'll match you with someone"
                        "once there is another person!\n";
                send(oss.str(), stream);
            }
        }

        if(splitCommand[0].compare("MSG") == 0){
            string msg = rec;
            if (&me != NULL)
                me.sendMessage(msg);

        }

    }
    delete stream;
}

void connectionManager(ClientQueue *c){
    for(;;) {
        TCPStream* stream = acceptor->accept();
        if (stream != NULL) {
            std::thread t1(new_connection, stream, c);
            t1.detach();
        }
    }
}