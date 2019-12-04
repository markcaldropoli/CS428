//FIXME reorganize
#include "cache.cpp"

#include <boost/algorithm/string.hpp>
#include <unordered_map>
#include <sstream>
#include <vector>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <iostream>
#include <chrono>
#include <ctime>

#define MAXNAMELEN 256

using namespace std;
using namespace chrono;

string getHttpUrl(string s) {
  string ret = "http://";
  int i = s.find("//")+2;
  while(i < s.length()) {
    if(s[i] == ':' || s[i] == '/') {
      break;
    }
    ret += s[i++];
  }
  return ret;
}
string getTrimmedHttpUrl(string s) {
  string ret = "";
  int i = s.find("//")+2;
  bool copyFlag = true;
  while(i < s.length()) {
    if(s[i] == ':')
	  copyFlag = false;
	if(s[i] == '/')
	  copyFlag = true;
	if(copyFlag)
	  ret += s[i];
	i++;
  }
  return ret;
}
//returns 80 if default port
int getPort(string s) {
  int start = s.find(":");
  if(s.find(":", start+1) == string::npos) {
    return 80;
  }
  int i = s.find(":", start+1)+1;

  //there is a ':' but no port number after it
  if(!isdigit(s[i])) {
    return 80;
  }

  int ret = 0;
  while(i < s.length()) {
    if(isdigit(s[i])) {
      ret *= 10;
      ret += s[i++] - '0';
    }
    else {
      break;
    }
  }
  return ret;
}
string getContentFromResponse(string response) {
  int start = response.find("Content-Length: ")+16;
  response = response.substr(start, response.size()-start);
  return response.substr(0, response.size()-(response.size()-response.find("\r\n")));
}
char* processHttpRequest(vector<string> headers, string http_url, ushort servport) {
  int sd;
  
  struct sockaddr_in sa;
  string host = http_url.substr(0, http_url.length()-(http_url.length()-http_url.find("/")));
  string header = "GET " + http_url.substr(http_url.find("/"), http_url.length()-host.length()) + "\r\n" + headers[1] + "\r\n" + headers[2] + "\r\n" + headers[3] + "\r\n" + "Host: " + host + "\r\n" + headers[4] + "\r\n" + headers[5] + "\r\n\r\n";

  if((sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == 0) {
    perror("socket failed");
    exit(0);
  }

  struct hostent *hostInfo = gethostbyname(host.c_str());
  if(!hostInfo) {
    perror("host by name failed");
    exit(0);
  }

  memset(&sa, 0, sizeof(sa));

  sa.sin_family = AF_INET;
  memcpy((char*)&sa.sin_addr.s_addr, (char*)hostInfo->h_addr, (hostInfo->h_length));
  sa.sin_port = htons(servport);

  memcpy(&sa.sin_addr.s_addr, hostInfo->h_addr, hostInfo->h_length);
  if(connect(sd, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
    perror("connect failed");
    exit(0);
  }

  if(send(sd, header.c_str(), strlen(header.c_str()), 0) < 0) {
    perror("send failed");
    exit(0);
  }

  char * buffer = (char*)malloc(1024*1024*2);
  long curr_processed = 0;

  for(;;) {
    int data_size = recv(sd, buffer+curr_processed, (1024*1024*2)-1, 0);
	if(data_size <= 0) {
	  break;
    }
	curr_processed += data_size;
  }

  close(sd);
  return buffer;
}

int startServer() {
  int sd;
  struct sockaddr_in sa;
  int addrlen = sizeof(sa);
  char * servhost;
  ushort servport;

  if ((sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == 0) {
    perror("socket failed");
    exit(0);
  }

  memset(&sa, 0, sizeof(sa));

  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  sa.sin_port = htons(0);

  if (bind(sd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
    perror("cannot bind");
    close(sd);
    exit(0);
  }

  listen(sd, 5); // FIXME backlog = 5?

  servhost = (char *)malloc(MAXNAMELEN);
  if(gethostname(servhost,MAXNAMELEN) == -1) {
    perror("cannot get host name");
    exit(0);
  }

  struct hostent* host = gethostbyname(servhost);
  if(!host) {
    perror("cannot get host address");
    exit(0);
  }

  memcpy(&sa.sin_addr.s_addr, host->h_addr, host->h_length);
  memcpy(servhost, host->h_name, MAXNAMELEN);

  struct sockaddr_in sa2;
  memset(&sa2,0,sizeof(sa2));

  socklen_t* len = (socklen_t *) sizeof(sa2);
  if(getsockname(sd, (struct sockaddr *) &sa2, (socklen_t *) &len) < 0) {
    perror("getsockname failed");
    return 0;
  }

  servport = ntohs(sa2.sin_port);

  printf("admin: started server on '%s' at '%hu'\n", servhost, servport);
  free(servhost);
  return (sd);
}

int main(int argc, char const *argv[]) {
  if(argc != 2) {
    cerr << "./proxy <cache_size>" << endl;
	exit(0);
  }
  
  LRUCache cache = LRUCache(stoi(argv[1]));
  

  int servsock;
  fd_set clientset;
  int livesdmax;

  servsock = startServer();
  if(servsock < 0) {
    perror("error starting server");
    exit(0);
  }

  FD_ZERO(&clientset);
  FD_SET(servsock, &clientset);
  livesdmax = servsock;

  for(;;) {
    select(livesdmax+1, &clientset, nullptr, nullptr, nullptr);

    //look for connect requests
    if(FD_ISSET(servsock, &clientset)) {
      struct sockaddr_in client;
      memset(&client, 0, sizeof(client));
      socklen_t len = sizeof(client);
      int csd = accept(servsock, (struct sockaddr*)&client, &len);

      //accept is good
      if(csd != -1) {
        auto start = system_clock::now();
        char* ip_addr = inet_ntoa(client.sin_addr);
        char* http_request = (char*)malloc(1024);
        read(csd, http_request, 1024);
        stringstream ss(http_request);
        string to;
        vector<string> http_headers;

        //populate http header vector with the correct information
        while(getline(ss, to, '\n')) {
		  boost::trim_right(to);
          if(!to.empty())
			http_headers.push_back(to);
        }

        //remove host name from header
        string host = http_headers[4];
        http_headers.erase(http_headers.begin()+4);
        string str = http_headers[0];

        string http_url = getHttpUrl(str);
        string trimmed_http_url = getTrimmedHttpUrl(str);
        int port = getPort(str);

		//check cache to see if entry is present
		bool cacheHit;
		char * response;
        //if not get http response from website
		if(!cache.contains(trimmed_http_url)) {
		  cacheHit = false;
		  response = processHttpRequest(http_headers,trimmed_http_url,port);
		}
		//if it is, get it from cache
		else {
		  cacheHit = true;
		  response = (char*)malloc(1024*1024*2);
		  response = cache.get(trimmed_http_url);
		  cache.updateOnHit(trimmed_http_url);
		}

		string cacheStatus = cacheHit ? "CACHE_HIT" : "CACHE_MISS";

        //extract content-length
		string contentLength = getContentFromResponse(response);
		
        //send to client
        write(csd, response, 1024*1024*2);

        //close client
        close(csd);
       
		//insert into cache
		if(!cacheHit) {
		  cache.insert(trimmed_http_url, response, stoi(contentLength));
		}
		
		//free response
		//free(response);

		//end timer
		auto end = system_clock::now();

        //calculate request time
        duration<double> sec = end - start;
        int msecs = duration_cast<milliseconds>(sec).count();

        //print assignment requirements
        cout << ip_addr << "|" << http_url << "|" << cacheStatus << "|" << contentLength << "|" << msecs << endl;
      } else {
        perror("accept failed");
        exit(0);
      }
    }
  }
}
