/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

#define err_quit(m) { perror(m); exit(-1); }

#define NIPQUAD(s)	((unsigned char *) &s)[0], \
					((unsigned char *) &s)[1], \
					((unsigned char *) &s)[2], \
					((unsigned char *) &s)[3]

static int s = -1;
static struct sockaddr_in sin;
static unsigned seq;
static unsigned count = 0;
string filename_arr[7][3] = { {"000000","000047","000094"},{"000143","000190","000237"},{"000286","000333","000380"},{"000429","000476","000523"},{"000572","000619","000666"},{"000715","000762","000809"},{"000858","000905","000952"}};
string filename;
string root;
bool sign[3] = {true,true,true};

typedef struct {
	unsigned seq;
	struct timeval tv;
}	ping_t;

double tv2ms(struct timeval *tv) {
	return 1000.0*tv->tv_sec + 0.001*tv->tv_usec;
}
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
void do_send(int sig) {
	char buf[1024];
	char se_buf[1030];
	string temp;
	if(sig == SIGALRM) {
			string fullname = root +"/"+ filename; 
			int fd = open(fullname.c_str(),O_RDONLY);
			while(read(fd,buf,1024))
			{
				sprintf(se_buf,"%d %s %s",seq,filename.c_str(),buf);
				if(sendto(s, se_buf, 1030, 0, (struct sockaddr*) &sin, sizeof(sin)) < 0)
					perror("sendto");
				seq++;
			}
	}
	for(int i = 5 ; i>=0 ; i--)
	{
		if(filename[i] == '9')
		{
			filename[i] = '0';
			continue;
		}
		filename[i]++;
		break;
	}
	count++;
	if(count > 1000) exit(0);
}

int main(int argc, char *argv[]) {
	if(argc < 3) {
		return -fprintf(stderr, "usage: %s ... <port> <ip>\n", argv[0]);
	}

	srand(time(0) ^ getpid());

	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	long aaa = strtol(argv[argc-2], NULL, 0);
	sin.sin_port = htons(aaa);
	int id = 0;
	pid_t pid = fork();
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
	if(inet_pton(AF_INET, argv[argc-1], &sin.sin_addr) != 1) {
			return -fprintf(stderr, "** cannot convert IPv4 address for %s\n", argv[1]);
	}
	if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		err_quit("socket");
	int val = 52500;
	struct timeval tv={0,val};
	setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
	fprintf(stderr, "PING %u.%u.%u.%u/%u, init seq = %d\n",
		NIPQUAD(sin.sin_addr), ntohs(sin.sin_port), seq);
	root = argv[1];
	//read the file to seq_to_data
	map <int,string> seq_to_data[3];
	bool mod_sign[3] = {true,true,true};
	for(int i = 0 ; i<3 ; i++)
	{
		int abc = 0;
		filename = filename_arr[id][i];
		while(1)
		{
			string fullname = root +"/"+ filename;
			int fd = open(fullname.c_str(),O_RDONLY);
			char buf[1000];
			for(int j = 0 ; j<1000 ; j++) buf[j] = '\0';
			while(read(fd,buf,1000))
			{
				char total[1024];
				for(int j = 0 ; j<1024 ; j++) total[j] = '\0';
				sprintf(total,"%d %s %s %d",abc,filename.c_str(),buf,i);
				string tmp1 = total;
				seq_to_data[i][abc] = tmp1;
				abc++; 
				for(int j = 0 ; j<1000 ; j++) buf[j] = '\0';
			}
			close(fd);
			for(int j = 5 ; j>=0 ; j--)
			{
				if(filename[j] == '9')
				{
					filename[j] = '0';
					continue;
				}
				filename[j]++;
				break;
			}
			if(filename == "000047" || filename == "000094" || filename == "000143") break;
			if(filename == "000190" || filename == "000237" || filename == "000286") break;
			if(filename == "000333" || filename == "000380" || filename == "000429") break;
			if(filename == "000476" || filename == "000523" || filename == "000572") break;
			if(filename == "000619" || filename == "000666" || filename == "000715") break;
			if(filename == "000762" || filename == "000809" || filename == "000858") break;
			if(filename == "000905" || filename == "000952" || filename == "001000") break;
		}
	}
	int cnt[3] = {0,0,0};
	char buf[1000];
	while(sign[0] || sign[1] || sign[2]) {
		int rlen;
		struct sockaddr_in csin;
		socklen_t csinlen = sizeof(csin);
		char s0 [1024];
		sprintf(s0,"%s",seq_to_data[0][cnt[0]].c_str());
		char s1 [1024];
		sprintf(s1,"%s",seq_to_data[1][cnt[1]].c_str());
		char s2 [1024];
		sprintf(s2,"%s",seq_to_data[2][cnt[2]].c_str());
		if(sign[0] && sendto(s, s0, 1024, 0, (struct sockaddr*) &sin, sizeof(sin)) < 0)
		{
			perror("sendto 0");
		}
		if(!sign[0])
		{
			if(mod_sign[0])
			{
				val -= 10000;
				struct timeval tv0={0,val};
				setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,&tv0,sizeof(tv0));
				mod_sign[0] = false;
			}
		}
		if(sign[1] && sendto(s, s1, 1024, 0, (struct sockaddr*) &sin, sizeof(sin)) < 0)
		{
			perror("sendto 1");
		}
		if(!sign[1])
		{
			if(mod_sign[1])
			{
				val -= 35000;
				struct timeval tv0={0,val};
				setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,&tv0,sizeof(tv0));
				mod_sign[1] = false;
			}
		}
		if(sign[2] && sendto(s, s2, 1024, 0, (struct sockaddr*) &sin, sizeof(sin)) < 0)
			perror("sendto 2");
		for(int i = 0 ; i<1000 ; i++) buf[i] = '\0';
		if((rlen = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*) &csin, &csinlen)) < 0) {
			//perror("recvfrom 1");
			continue;
		}
		string tmp;
		vector <string> pkg;
		tmp = buf;
		pkg = getcmd(tmp);
		if(pkg[0] == "0" && sign[0])
		{
			int a = atoi(pkg[1].c_str());
			if(seq_to_data[0].count(a))
			{
				cnt[0] = atoi(pkg[1].c_str());
			}
			else sign[0] = false;
		}
		if(pkg[0] == "1" && sign[1])
		{
			int a = atoi(pkg[1].c_str());
			if(seq_to_data[1].count(a))
			{
				cnt[1] = atoi(pkg[1].c_str());
			}
			else sign[1] = false;
		}
		if(pkg[0] == "2" && sign[2])
		{
			int a = atoi(pkg[1].c_str());
			if(seq_to_data[2].count(a))
			{
				cnt[2] = atoi(pkg[1].c_str());
			}
			else sign[2] = false;
		}
	}
	close(s);
	return 0;
}