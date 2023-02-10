/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <vector>
#include <iostream>
#include <map>
#include <stdio.h>
using namespace std;
#define err_quit(m) { perror(m); exit(-1); }
string filename = "000000";
string root;
map <string,int> loc;
vector <string> getcmd(string s)
{
	int pre = 0;
	vector <string> ans;
	for(int i = 0; i<s.length() ; i++)
	{
		if(s[i] == ' ')
		{
			ans.push_back(s.substr(pre,i-pre));
			pre = i+1;
		}
	}
	ans.push_back(s.substr(pre,s.length()-pre));
	return ans;
}
int main(int argc, char *argv[]) {
	int s;
	struct sockaddr_in sin;
	if(argc < 2) {
		return -fprintf(stderr, "usage: %s ... <port>\n", argv[0]);
	}

	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	long aaa = strtol(argv[argc-1], NULL, 0);
	sin.sin_port = htons(aaa);
	pid_t pid = fork();
	int id = 0;
	if(pid == 0)
	{
		aaa++;
		sin.sin_port = htons(aaa);
		pid = fork();
		id = 1;
		if(pid == 0)
		{
			aaa++;
			sin.sin_port = htons(aaa);
			pid = fork();
			id = 2;
			if(pid == 0)
			{
				aaa++;
				sin.sin_port = htons(aaa);
				pid = fork();
				id = 3;
				if(pid == 0)
				{
					aaa++;
					sin.sin_port = htons(aaa);
					pid = fork();
					id = 4;
					if(pid == 0)
					{
						aaa++;
						sin.sin_port = htons(aaa);
						pid = fork();
						id = 5;
						if(pid == 0)
						{
							aaa++;
							sin.sin_port = htons(aaa);
							id = 6;
						}
					}
				}
			}
		}
	}

	if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		err_quit("socket");

	if(bind(s, (struct sockaddr*) &sin, sizeof(sin)) < 0)
		err_quit("bind");
	int want_seq[3] = {0,0,0};
	root = argv[1];
	int fd;
	while(1) {
		struct sockaddr_in csin;
		socklen_t csinlen = sizeof(csin);
		char buf[1024];
		int rlen;
		int pkg_len;
		for(int i = 0 ; i<1024 ; i++) buf[i] = '\0';
		if((rlen = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*) &csin, &csinlen)) < 0) {
			perror("recvfrom");
			break;
		}
		if(rlen<1024)
		{
			sprintf(buf,"0");
			cout << "The size is " << rlen << endl;
			sendto(s, buf, 1024, 0, (struct sockaddr*) &csin, sizeof(csin));
		}
		else
		{
			string tmp = buf;
			vector <string> pkg;
			string fullname="";
			pkg = getcmd(tmp);
			char data [1024];
			for(int i = 0 ; i<1024 ; i++) data[i] = '\0';
			sprintf(data,"%s", pkg[2].c_str());
			int cnt = atoi(pkg[3].c_str());
			int seq = atoi(pkg[0].c_str());
			if(want_seq[cnt] == seq)
			{
				fullname = root+"/"+pkg[1];
				if(loc.count(fullname) == 0)
				{
					int mode = S_IRWXU | S_IRWXG  | S_IRWXO;
					fd = open(fullname.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0777);
					loc[fullname] = fd;
				}
				lseek(loc[fullname],0,SEEK_END);
				int size = pkg[2].size();
				if(write(loc[fullname],data,size)<0)
					perror("write");
				want_seq[cnt]++;
			}
			for(int i = 0 ; i<1024 ; i++) buf[i] = '\0';
			sprintf(buf,"%d %d",cnt,want_seq[cnt]);
			sendto(s, buf, 1024, 0, (struct sockaddr*) &csin, sizeof(csin));
		}
	}

	close(s);
}