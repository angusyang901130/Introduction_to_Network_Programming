#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/udp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#define MAX_SIZE 1024
#define NAME_LENGTH 257
#define A 1
#define AAAA 28
#define NS 2
#define SOA 6
#define MX 15
#define TXT 16
#define CNAME 5

char* nipio_pattern = "^([0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3}).([0-9a-zA-Z]{1,61}.)*$";
//char* ip_pattern = "^[0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3}$";

typedef struct {
    int16_t ID;
    uint16_t RD : 1;
    uint16_t TC : 1;
    uint16_t AA : 1;
    uint16_t OPCODE : 4;
    uint16_t QR : 1;
    uint16_t RCODE : 4;
    uint16_t Z : 3;
    uint16_t RA : 1;
    uint16_t QDCOUNT;
    uint16_t ANCOUNT;
    uint16_t NSCOUNT;
    uint16_t ARCOUNT;
}__attribute__((packed)) Header_t;

typedef struct {
    uint8_t* QNAME;
    uint16_t QTYPE;
    uint16_t QCLASS;
}__attribute__((packed)) Question_t;

typedef struct {
    uint8_t* NAME;
    uint16_t TYPE;
    uint16_t CLASS;
    uint32_t TTL;
    uint16_t RDLENGTH;
    uint8_t* RDATA; 
}__attribute__((packed)) Record_t;

typedef struct {
    Header_t* header;
    Question_t* question;
    Record_t* records;
}__attribute__((packed)) Msg_t;

void ShowHeader(Header_t* header){
    printf("Header ID: %u\n", ntohs(header->ID));
    printf("Header QR: %u, OPCODE: %u, AA: %u, TC: %u, RD: %u, RA: %u, Z: %u, RCODE: %u\n", header->QR, header->OPCODE, \
            header->AA, header->TC, header->RD, header->RA, header->Z, header->RCODE);
    printf("Header QDCOUNT: %u\n", ntohs(header->QDCOUNT));
    printf("Header ANCOUNT: %u\n", ntohs(header->ANCOUNT));
    printf("Header NSCOUNT: %u\n", ntohs(header->NSCOUNT));
    printf("Header ARCOUNT: %u\n", ntohs(header->ARCOUNT));
}

void ShowQuestion(Question_t* ques){
    printf("Question NAME: %s\n", ques->QNAME);
    printf("Question TYPE: %u\n", ntohs(ques->QTYPE));
    printf("Question CLASS: %u\n", ntohs(ques->QCLASS));
}

void ShowAnswer(Record_t* ans){
    printf("Answer NAME: %s\n", ans->NAME);
    printf("Answer TYPE: %u\n", ntohs(ans->TYPE));
    printf("Answer CLASS: %u\n", ntohs(ans->CLASS));
    printf("Answer TTL: %u\n", ntohl(ans->TTL));
    printf("Answer RDLENGTH: %u\n", ntohs(ans->RDLENGTH));
    printf("Answer RDATA: %s\n", ans->RDATA);
}

void ShowAuthority(Record_t* auth){
    printf("Authority NAME: %s\n", auth->NAME);
    printf("Authority TYPE: %u\n", ntohs(auth->TYPE));
    printf("Authority CLASS: %u\n", ntohs(auth->CLASS));
    printf("Authority TTL: %u\n", ntohl(auth->TTL));
    printf("Authority RDLENGTH: %u\n", ntohs(auth->RDLENGTH));
    printf("Authority RDATA: %s\n", auth->RDATA);
}

void ShowAdditional(Record_t* add){
    printf("Additional NAME: %s\n", add->NAME);
    printf("Additional TYPE: %u\n", ntohs(add->TYPE));
    printf("Additional CLASS: %u\n", ntohs(add->CLASS));
    printf("Additional TTL: %u\n", ntohl(add->TTL));
    printf("Additional RDLENGTH: %u\n", ntohs(add->RDLENGTH));
    printf("Additional RDATA: %s\n", add->RDATA);
}

int parseName(uint8_t* name, char* domain, uint8_t* qname){
    //qname = (uint8_t*)realloc(qname, strlen(domain)+1);
    qname[0] = *name;
    name++;
    //char domain[NAME_LENGTH+1];
    int pos = 0, len = 1;

    while(*name != 0){
        //printf("current name: %u\n", *name);
        //qname = realloc(qname, len+1);
        qname[pos+1] = *name;

        if(*((char*)name) >= 'A' && *((char*)name) <= 'Z'){
            domain[pos] = *((char*)name);
        }else if(*((char*)name) >= 'a' && *((char*)name) <= 'z'){
            domain[pos] = *((char*)name);
        }else if(*((char*)name) >= '0' && *((char*)name) <= '9'){
            domain[pos] = *((char*)name);
        }else{
            domain[pos] = '.';
        }

        //printf("current pos: %d, char: %c, value: %u\n", pos, domain[pos], qname[pos+1]);
        pos++;
        len++;
        name++;
    }
    
    domain[pos] = '.';
    //qname = realloc(qname, len+1);
    qname[pos+1] = *name;

    return len;
}

int parseLine(char* line){
    int pos = 0;
    while(*line != ','){
        pos++;
        line++;
    }
    pos++;
    return pos;
}

int getDomainFromSub(char* sub_domain, char* doc_domain){
    int pos = 0;

    while(*(sub_domain+pos) != '.'){
        pos++;
    }
    
    if(strcmp(sub_domain+pos+1, doc_domain) == 0)
        return pos+1;
    else return 0;
}

int getType(char* class){
    int value;
    if(strcmp(class, "A") == 0)
        value = A;
    else if(strcmp(class, "NS") == 0)
        value = NS;
    else if(strcmp(class, "SOA") == 0)
        value = SOA;
    else if(strcmp(class, "MX") == 0)
        value = MX;
    else if(strcmp(class, "TXT") == 0)
        value = TXT;
    else if(strcmp(class, "AAAA") == 0)
        value = AAAA;
    else if(strcmp(class, "CNAME") == 0)
        value = CNAME;
    
    return value;
}

void compress(char* domain, uint8_t* compressed){
    int cnt = 0;

    //printf("domain: %s\n", domain);
    int name_len = strlen(domain);
    int ind = 1;
    //printf("%d\n", (int)domain[14]);
    while(ind <= name_len){
        if(*domain == '.'){
            *(compressed+ind-cnt-1) = (uint8_t)cnt;
            //printf("count: %d\n", cnt);
            cnt = 0;
        }else{
            cnt++;
            *(compressed+ind) = *(uint8_t*)(domain);
            //printf("ind: %d, char: %c\n", ind, *(char*)(compressed+ind));
        }
    
        domain++;
        ind++;
    }
    *(compressed+ind-1) = 0;
}

void fillName(char* buf, uint8_t* compressed_name, int name_len){
    //printf("aaa\n");
    uint8_t* name_ptr = (uint8_t*)buf;
    //printf("name_len: %d\n", name_len);
    for(int i = 0; i < name_len; i++){
        //printf("char: %c\n", *(char*)(compressed_name+i));
        *(name_ptr+i) = *(compressed_name+i);
    }
}

void fillInfo(char* buf, uint16_t type, uint16_t class, uint32_t ttl){
    uint16_t* type_ptr = (uint16_t*)buf; 
    uint16_t* class_ptr = (uint16_t*)(buf + 2);
    uint32_t* ttl_ptr = (uint32_t*)(buf + 4);

    *type_ptr = htons(type);
    *class_ptr = htons(class);
    *ttl_ptr = htonl(ttl);
}

int fillHeader(char* buf, Header_t* header){
    int size = 0;
    Header_t* h = (Header_t*)buf;
    h->ID = header->ID;
    h->RD = header->RD;
    h->TC = header->TC;
    h->AA = header->AA;
    h->OPCODE = header->OPCODE;
    h->QR = htons(1);
    h->RCODE = header->RCODE;
    h->Z = header->Z;
    h->RA = header->RA;
    h->QDCOUNT = header->QDCOUNT;
    h->ANCOUNT = header->ANCOUNT;
    h->NSCOUNT = header->NSCOUNT;
    h->ARCOUNT = header->ARCOUNT;
    return (int)sizeof(Header_t);
}

int fillQuestion(char* buf, Question_t ques, int name_len){
    int size = 0;
    uint8_t* name_ptr = (uint8_t*)buf;
    for(int i = 0; i < name_len; i++){
        *(name_ptr+i) = *(ques.QNAME+i);
    }
    uint16_t* info_ptr = (uint16_t*)(buf+name_len);
    *info_ptr = ques.QTYPE;
    *(info_ptr+1) = ques.QCLASS;
    return name_len + 4;
}

int fillSOA(char* buf, char* data){
    int soaByte = 20;
    char* token = strtok(data, " ");
    uint8_t* mname = (uint8_t*)buf;
    compress(token, mname);
    soaByte += strlen(token) + 1;

    uint8_t* rname = (uint8_t*)(buf + strlen(token) + 1);
    token = strtok(NULL, " ");
    compress(token, rname);
    soaByte += strlen(token) + 1;

    uint32_t* serial = (uint32_t*)(rname + strlen(token) + 1);
    token = strtok(NULL, " ");
    *serial = htonl((uint32_t)atoi(token));

    uint32_t* refresh = (uint32_t*)(serial + 1);
    token = strtok(NULL, " ");
    *refresh = htonl((uint32_t)atoi(token));

    uint32_t* retry = (uint32_t*)(refresh + 1);
    token = strtok(NULL, " ");
    *retry = htonl((uint32_t)atoi(token));

    uint32_t* expire = (uint32_t*)(retry + 1);
    token = strtok(NULL, " ");
    *expire = htonl((uint32_t)atoi(token));

    uint32_t* minimum = (uint32_t*)(expire + 1);
    token = strtok(NULL, " ");
    *minimum = htonl((uint32_t)atoi(token));

    return soaByte;
}

int fillAnswer(char* buf, FILE* doc, char* domain, uint8_t* compressed_domain, int flag, int qtype, int* cnt, char* cname){
    char* line = NULL;
    ssize_t nbytes;
    size_t len = 0;
    uint16_t type, class, rdlength;
    uint32_t ttl;
    int ans_bytes = 0;
    int name_len = strlen(domain)+1;
    
    //printf("Request qtype: %d\n", qtype);
    // no service
    if(flag){
        //printf("No service is required\n");
        while((nbytes = getline(&line, &len, doc)) != -1){
            //printf("Read a line\n");
            if(*line == '@'){
                // domain name
                char* token = strtok(line, ",");
                token = strtok(NULL, ",");
                ttl = (uint32_t)atoi(token);

                token = strtok(NULL, ",");
                class = 1;

                token = strtok(NULL, ",");
                type = getType(token);
                if(type != qtype)
                    continue;

                printf("Found matched line\n");
                *cnt = *cnt + 1;
                token = strtok(NULL, ",");
                if(token[strlen(token)-1] == '\n')
                    token[strlen(token)-1] = '\0';

                uint16_t* rd_len;
                uint32_t* rdata;
                int section_bytes = 0;
                int conv;

                switch(type){
                    case A:
                        // only address is left
                        fillName(buf, compressed_domain, name_len);
                        fillInfo(buf+name_len, type, class, ttl);
                        rd_len = (uint16_t*)(buf+name_len+8);
                        *rd_len = htons(4);
                        //in_addr_t addr = inet_addr(token);
                        rdata = (uint32_t*)(buf+name_len+10);
                        conv = inet_pton(AF_INET, token, rdata);
                        if(conv != 1)
                            printf("Error during converting ip address\n");

                        section_bytes = name_len + 10 + (int)strlen(token) + 2;
                        ans_bytes += section_bytes;
                        buf += section_bytes;
                        break;

                    case AAAA:
                        // only address is left
                        fillName(buf, compressed_domain, name_len);
                        fillInfo(buf+name_len, type, class, ttl);
                        rd_len = (uint16_t*)(buf+name_len+8);
                        *rd_len = htons(16);
                        //in_addr_t addr = inet_addr(token);
                        rdata = (uint32_t*)(buf+name_len+10);
                        conv = inet_pton(AF_INET6, token, rdata);
                        if(conv != 1)
                            printf("Error during converting ip address\n");

                        section_bytes = name_len + 10 + (int)strlen(token) + 2;
                        ans_bytes += section_bytes;
                        buf += section_bytes;
                        break;

                    case CNAME:
                        // only canonical is left
                        fillName(buf, compressed_domain, name_len);
                        fillInfo(buf+name_len, type, class, ttl);
                        compress(token, buf+name_len+10);
                        rd_len = (uint16_t*)(buf+name_len+8);
                        *rd_len = htons((uint16_t)strlen(token)+1);

                        section_bytes = name_len + 10 + (int)strlen(token) + 1;
                        ans_bytes += section_bytes;
                        buf += section_bytes;
                        break; 

                    case NS:
                        // only DNS server domain is left
                        printf("NS section\n");
                        fillName(buf, compressed_domain, name_len);
                        fillInfo(buf+name_len, type, class, ttl);
                        compress(token, buf+name_len+10);
                        rd_len = (uint16_t*)(buf+name_len+8);
                        //printf("aaaaa\n");
                        *rd_len = htons((uint16_t)strlen(token)+1);
                        //printf("bbbbb\n");
                        //printf("length: %d\n", (int)strlen(token));
                        section_bytes = name_len + 10 + (int)strlen(token) + 1;
                        //printf("ccccc\n");
                        ans_bytes += section_bytes;
                        buf += section_bytes;
                        break; 

                    case SOA:
                        // a lot of elements left
                        fillName(buf, compressed_domain, name_len);
                        fillInfo(buf+name_len, type, class, ttl);
                        int soa_byte = fillSOA(buf+name_len+10, token);
                        rd_len = (uint16_t*)(buf+name_len+8);
                        *rd_len = htons((uint16_t)soa_byte);

                        section_bytes = name_len + 10 + soa_byte;
                        ans_bytes += section_bytes;
                        buf += section_bytes;
                        break; 

                    case TXT:
                        // only text is left
                        fillName(buf, compressed_domain, name_len);
                        fillInfo(buf+name_len, type, class, ttl);
                        uint8_t* txt = strtok(token, "\"");
                        //uint8_t* txt = (uint8_t*)token;
                        printf("txt: %s\n", txt);
                        //compress(txt, buf+name_len+10);
                        rd_len = (uint16_t*)(buf+name_len+8);
                        uint8_t* txtToInt = (uint8_t*)(buf+name_len+10);
                        *txtToInt = (uint8_t)strlen(txt);
                        strcpy(txtToInt+1, txt);
                        //printf("txt: %s\n", buf+name_len+10+strlen(txt));
                        *rd_len = htons((uint16_t)strlen(txt) + 1);

                        section_bytes = name_len + 10 + (int)strlen(txt) + 1;
                        ans_bytes += section_bytes;
                        buf += section_bytes;
                        break; 

                    case MX:
                        // there is a number and a domain
                        fillName(buf, compressed_domain, name_len);
                        fillInfo(buf+name_len, type, class, ttl);

                        uint16_t* pref_ptr = (uint16_t*)(buf+name_len+10);
                        char* r_tok = strtok(token, " ");
                        *pref_ptr = htons((uint16_t)atoi(r_tok));
                        //printf("%d\n", ntohs(*pref_ptr));

                        r_tok = strtok(NULL, " ");
                        //printf("%s\n", r_tok);
                        compress(r_tok, buf+name_len+12);
                        rd_len = (uint16_t*)(buf+name_len+8);
                        *rd_len = htons((uint16_t)strlen(r_tok)+1+2);

                        section_bytes = name_len + 10 + (int)strlen(r_tok) + 1 + 2;
                        ans_bytes += section_bytes;
                        //printf("ans_bytes: %d\n", ans_bytes);
                        buf += section_bytes;
                        break; 
                }

            } 
        }
        //printf("Finsh answer section\n");     
    }else if(!flag){
        nbytes = getline(&line, &len, doc);
        char* query_serv = strtok(domain, ".");
        //printf("service: %s\n", query_serv);
        while((nbytes = getline(&line, &len, doc)) != -1){
            if(*line == '@')
                continue;
            
            char* serv = strtok(line, ",");
            //printf("query_serv: %s, serv: %s\n", query_serv, serv);
            //printf("aaaa\n");
            if(strcmp(query_serv, serv) == 0){
                //printf("aaa\n");
                //printf("line: %s\n", line);
                //char* token = strtok(NULL, ",");
                //printf("token: %s\n", token);
                char* token = strtok(NULL, ",");
                //printf("token: %s\n", token);
                ttl = (uint32_t)atoi(token);

                token = strtok(NULL, ",");
                //printf("token: %s\n", token);
                class = 1;

                token = strtok(NULL, ",");
                //printf("token: %s\n", token);
                type = getType(token);

                if(type != qtype)
                    continue;

                *cnt = *cnt + 1;
                token = strtok(NULL, ",");
                
                if(token[strlen(token)-1] == '\n')
                    token[strlen(token)-1] = '\0';

                //printf("token: %s\n", token);

                uint16_t* rd_len;
                uint32_t* rdata;
                int section_bytes = 0;
                int conv;

                switch(type){
                    case A:
                        // only address is left
                        //printf("aaaa\n");
                        fillName(buf, compressed_domain, name_len);
                        fillInfo(buf+name_len, type, class, ttl);
                        rd_len = (uint16_t*)(buf+name_len+8);
                        *rd_len = htons(4);
                        //in_addr_t addr = inet_addr(token);
                        rdata = (uint32_t*)(buf+name_len+10);
                        conv = inet_pton(AF_INET, token, rdata);
                        if(conv != 1)
                            printf("Error during converting ip address\n");

                        section_bytes = name_len + 10 + 4;
                        ans_bytes += section_bytes;
                        buf += section_bytes;
                        break;

                    case AAAA:
                        // only address is left
                        fillName(buf, compressed_domain, name_len);
                        fillInfo(buf+name_len, type, class, ttl);
                        rd_len = (uint16_t*)(buf+name_len+8);
                        *rd_len = htons(16);
                        //in_addr_t addr = inet_addr(token);
                        rdata = (uint32_t*)(buf+name_len+10);
                        conv = inet_pton(AF_INET6, token, rdata);
                        if(conv != 1)
                            printf("Error during converting ip address\n");

                        section_bytes = name_len + 10 + 16;
                        ans_bytes += section_bytes;
                        buf += section_bytes;
                        break;

                    case CNAME:
                        // only canonical is left
                        //printf("cname: %s\n", compressed_domain);
                        fillName(buf, compressed_domain, name_len);
                        fillInfo(buf+name_len, type, class, ttl);
                        compress(token, buf+name_len+10);
                        //printf("%s\n", buf+name_len+10);

                        cname = (char*)realloc(cname, strlen(token)+1);
                        //printf("token: %s\n", token);
                        strcpy(cname, token);

                        rd_len = (uint16_t*)(buf+name_len+8);
                        *rd_len = htons((uint16_t)strlen(token)+1);

                        section_bytes = name_len + 10 + (int)strlen(token) + 1;
                        ans_bytes += section_bytes;
                        buf += section_bytes;
                        break; 
                }
            }
        }
    }
    
    return ans_bytes;
}

int fillAuthority(char* buf, FILE* doc, uint8_t* compressed_domain, int qtype, int name_len, int* cnt){
    ssize_t nbytes;
    size_t len = 0;
    char* line;
    uint16_t type, class, rdlength;
    uint32_t ttl;
    int ans_bytes = 0;
    //printf("qtype: %d\n", qtype);
    
    while((nbytes = getline(&line, &len, doc)) != -1){
        //printf("Read a line\n");
        if(*line == '@'){
            // domain name
            char* token = strtok(line, ",");
            token = strtok(NULL, ",");
            ttl = (uint32_t)atoi(token);

            token = strtok(NULL, ",");
            class = 1;

            token = strtok(NULL, ",");
            
            type = getType(token);
            if(type != qtype)
                continue;

            *cnt = *cnt + 1;
            token = strtok(NULL, ",");
            if(token[strlen(token)-1] == '\n')
                token[strlen(token)-1] = '\0';
            //printf("token: %s\n", token);
            uint16_t* rd_len;
            int section_bytes = 0;

            switch(type){
                case NS:
                        // only DNS server domain is left
                    fillName(buf, compressed_domain, name_len);
                    fillInfo(buf+name_len, type, class, ttl);
                    compress(token, buf+name_len+10);
                    rd_len = (uint16_t*)(buf+name_len+8);
                    *rd_len = htons((uint16_t)strlen(token)+1);

                    section_bytes = name_len + 10 + (int)strlen(token) + 1;
                    ans_bytes += section_bytes;
                    buf += section_bytes;
                    break; 
                
                case SOA:
                    // a lot of elements left
                    //printf("soa\n");
                    fillName(buf, compressed_domain, name_len);
                    fillInfo(buf+name_len, type, class, ttl);
                    int soa_byte = fillSOA(buf+name_len+10, token);
                    rd_len = (uint16_t*)(buf+name_len+8);
                    *rd_len = htons((uint16_t)soa_byte);

                    section_bytes = name_len + 10 + soa_byte;
                    ans_bytes += section_bytes;
                    buf += section_bytes;
                    break;
            }
        }
    }

    return ans_bytes;         
}

int fillAddition(char* buf, FILE* doc, char* domain, uint8_t* compressed_domain, int* cnt){
    //printf("aaa\n");
    ssize_t nbytes;
    size_t len = 0;
    char* line;
    uint16_t type, class, rdlength;
    uint32_t ttl;
    int ans_bytes = 0;
    int name_len = strlen(domain) + 1;

    nbytes = getline(&line, &len, doc);
    char* tmp_domain = (char*)calloc(30, sizeof(char));
    strcpy(tmp_domain, domain);

    char* query_serv = strtok(domain, ".");

    while((nbytes = getline(&line, &len, doc)) != -1){
        if(*line == '@')
            continue;
        
        char* serv = strtok(line, ",");
        //printf("query_serv: %s, serv: %s\n", query_serv, serv);
        if(strncmp(query_serv, serv, strlen(query_serv)) == 0){

            char* token = strtok(NULL, ",");
            ttl = (uint32_t)atoi(token);

            token = strtok(NULL, ",");
            class = 1;
            
            token = strtok(NULL, ",");
            type = getType(token);

            *cnt = *cnt + 1;
            token = strtok(NULL, ",");
            if(token[strlen(token)-1] == '\n')
                token[strlen(token)-1] = '\0';
                
            uint16_t* rd_len;
            uint32_t* rdata;
            int section_bytes = 0;
            int conv;

            char* query_domain = (char*)calloc(30, sizeof(char));
            strcpy(query_domain, serv);
            //printf("query_domain: %s\n", query_domain);
            strcpy(query_domain+strlen(serv), tmp_domain+strlen(query_serv));
            //printf("query_domain: %s\n", query_domain);
            uint8_t* new_compress = calloc(strlen(query_domain)+1, sizeof(uint8_t));
            name_len = strlen(query_domain) + 1;
            compress(query_domain, new_compress);

            switch(type){
                case A:
                    // only address is left
                    
                    fillName(buf, new_compress, name_len);
                    fillInfo(buf+name_len, type, class, ttl);
                    
                    rd_len = (uint16_t*)(buf+name_len+8);
                    *rd_len = htons(4);
                    //in_addr_t addr = inet_addr(token);
                    rdata = (uint32_t*)(buf+name_len+10);
                    conv = inet_pton(AF_INET, token, rdata);
                    
                    if(conv != 1)
                        printf("Error during converting ip address\n");

                    section_bytes = name_len + 10 + 4;
                    ans_bytes += section_bytes;
                    buf += section_bytes;
                    break;

                case AAAA:
                    // only address is left
                    fillName(buf, new_compress, name_len);
                    fillInfo(buf+name_len, type, class, ttl);
                    rd_len = (uint16_t*)(buf+name_len+8);
                    *rd_len = htons(16);
                    //in_addr_t addr = inet_addr(token);
                    rdata = (uint32_t*)(buf+name_len+10);
                    conv = inet_pton(AF_INET6, token, rdata);
                    if(conv != 1)
                        printf("Error during converting ip address\n");

                    section_bytes = name_len + 10 + 16;
                    ans_bytes += section_bytes;
                    buf += section_bytes;
                    break;

                default:
                    break;
            }
        }
    }

    return ans_bytes;
}

int matchRexex(char* domain, char* pattern, char* match_part){
    regex_t preg;
    int success;
    if((success = regcomp(&preg, nipio_pattern, REG_EXTENDED|REG_ICASE)) != 0)
        printf("Compile Regex Error\n");

    regmatch_t matchptr[2];
    const size_t nmatch = 2;
    int status = regexec(&preg, domain, nmatch, matchptr, 0);

    if (status == REG_NOMATCH){ 
        printf("Pattern No Match\n");
        return 0;
    }else if (status == 0){  
        printf("Pattern Match\n");
        for (int i = matchptr[1].rm_so; i < matchptr[1].rm_eo; i++){  
            printf("%c", domain[i]);
        }

        int begin = matchptr[1].rm_so;
        int end = matchptr[1].rm_eo;
        memset(match_part, 0, end-begin+1);
        strncpy(match_part, domain+begin, end-begin);
        printf("\n");
        return 1;
    }
}

int fillNipAnswer(char* buf, uint8_t* compressed_domain, uint32_t ip, int name_len){
    int ans_bytes = 0;
    fillName(buf, compressed_domain, name_len);
    fillInfo(buf+name_len, A, 1, 1);
    uint16_t* rd_len = (uint16_t*)(buf+name_len+8);
    *rd_len = htons(4);

    uint32_t* rdata = (uint32_t*)(buf+name_len+10);
    *rdata = ip;

    ans_bytes = name_len + 10 + 4;
    return ans_bytes;
}

int main(int argc, char* argv[]){
    int sockfd;
    int rlen, slen;
    size_t len = 0;
    ssize_t nbytes = 0;
    struct sockaddr_in sin, csin, fsin;
    socklen_t csinlen = sizeof(csin);
    socklen_t fsinlen = sizeof(fsin);
    int header_bytes, ques_bytes, ans_bytes, auth_bytes, add_bytes;

    memset(&sin, 0, sizeof(sin));
    int port = atoi(argv[1]);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");

    if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        printf("Create Socket Error!\n");

    if(bind(sockfd, (struct sockaddr*) &sin, sizeof(sin)) < 0)
        printf("Bind Error!\n");

    while(1){
        char buf[MAX_SIZE];
        memset(buf, 0, MAX_SIZE);
        bzero(&csin, sizeof(csin));
        bzero(&fsin, sizeof(fsin));

        if((rlen = recvfrom(sockfd, buf, MAX_SIZE, 0, (struct sockaddr*)&csin, &csinlen)) < 0)
            printf("Receiving Error!\n");
    
        // Receiving dig client
        Header_t* header = (Header_t*)buf;

        Question_t ques;
        uint8_t* name_ptr = (uint8_t*)(buf + sizeof(Header_t));
        ques.QNAME = (uint8_t*)calloc(strlen(name_ptr)+1, sizeof(uint8_t));
        char* domain = (char*)calloc(NAME_LENGTH+1, sizeof(char));
        
        int ques_len = parseName(name_ptr, domain, ques.QNAME);
        printf("domain is %s, length: %d\n", domain, ques_len);
        ques.QTYPE = *((uint16_t*)(buf + sizeof(Header_t) + ques_len + 1));
        ques.QCLASS = *((uint16_t*)(buf + sizeof(Header_t) + ques_len + 1 + 2));  

        char* tmp_domain = calloc(strlen(domain)+1, sizeof(char));
        strcpy(tmp_domain, domain);
        //printf("tmp domain: %s\n", tmp_domain);

        ShowHeader(header);
        ShowQuestion(&ques);

        // Send msg to client or forward to other dns server
        char res[1024];

        // Handle nipio
        char* match_part = calloc(strlen(domain), sizeof(char));
        int match = matchRexex(tmp_domain, nipio_pattern, match_part);
        if(match){
            in_addr_t query_ip = inet_addr(match_part);
            header->ANCOUNT = htons(1);
            header->ARCOUNT = htons(0);
            header_bytes = fillHeader(res, header); 
            ques_bytes = fillQuestion(res+header_bytes, ques, ques_len+1);
            ans_bytes = fillNipAnswer(res+header_bytes+ques_bytes, ques.QNAME, query_ip, strlen(domain)+1);

            if((slen = sendto(sockfd, res, header_bytes+ques_bytes+ans_bytes, 0, (struct sockaddr*)&csin, csinlen)) < 0)
                printf("Send packet Problem\n");

            continue;
        }
    
        // First, read from config file
        FILE* conf_file = fopen(argv[2], "r");
        char* line = NULL;
        char* forwarded_ip = NULL;
        nbytes = getline(&forwarded_ip, &len, conf_file);
        forwarded_ip[strlen(forwarded_ip)-1] = '\0';
        printf("Foreign IP: %s\n", forwarded_ip);
        char* document = NULL;
        char* query_domain = NULL;

        while((nbytes = getline(&line, &len, conf_file)) != -1){
            int doc_pos = parseLine(line);
            char* doc_domain = strtok(line, ",");
            query_domain = domain + getDomainFromSub(domain, doc_domain);
            if(strncmp(query_domain, line, strlen(query_domain)) == 0){
                document = line + doc_pos;
                break;
            }
        }

        fclose(conf_file);
        
        // For domain not in config
        
        if(document == NULL){
            fsin.sin_addr.s_addr = inet_addr(forwarded_ip);
            //printf("ip: %u\n", cfsin.sin_addr.s_addr);
            fsin.sin_family = AF_INET;
            fsin.sin_port = htons(53);

            int new_sockfd;
            if((new_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
                printf("Create New Socket Error!\n"); 

            /* if(bind(new_sockfd, (struct sockaddr*)&fsin, sizeof(fsin)) < 0)
                printf("New Bind Error!\n"); */
            
            if((slen = sendto(new_sockfd, buf, rlen, 0, (struct sockaddr*)&fsin, sizeof(fsin))) < 0)
                printf("Forwarding Problem\n");

            if((rlen = recvfrom(new_sockfd, res, 1024, 0, (struct sockaddr*)&fsin, &fsinlen)) < 0)
                printf("Receiving packet from Forwarded IP Problem\n");

            if((slen = sendto(sockfd, res, 1024, 0, (struct sockaddr*)&csin, csinlen)) < 0)
                printf("Sending packet back to client Problem\n");
            
            continue;
        }

        //printf("tmp domain: %s\n", tmp_domain);

        // Second, after find the document, open document
        if(document[strlen(document)-1] == '\n')
            document[strlen(document)-1] = '\0';

        printf("Open document %s\n", document);
        FILE* doc_file = fopen(document, "r");
        if(doc_file == NULL)
            printf("Fail to open document\n");

        int flag = 0;
        if(strncmp(query_domain, domain, strlen(query_domain)) == 0)
            flag = 1;

        int ans_cnt = 0, auth_cnt = 0, add_cnt = 0;
        printf("Filling Header\n");
        header_bytes = fillHeader(res, header);
        printf("Filling Question\n");
        ques_bytes = fillQuestion(res+header_bytes, ques, ques_len+1);
        printf("Filling Answer\n");

        char* cname = (char*)calloc(1, sizeof(char));
        ans_bytes = fillAnswer(res+header_bytes+ques_bytes, doc_file, tmp_domain, ques.QNAME,\
                                    flag, ntohs(ques.QTYPE), &ans_cnt, NULL);

        //printf("ans_bytes: %d\n", ans_bytes);
        rewind(doc_file);

        memset(tmp_domain, 0, strlen(domain));
        strcpy(tmp_domain, domain);

        if(ans_cnt == 0 && (ntohs(ques.QTYPE) == A || ntohs(ques.QTYPE) == AAAA)){
            int sub_ans_bytes = fillAnswer(res+header_bytes+ques_bytes, doc_file, tmp_domain, ques.QNAME,\
                                    flag, CNAME, &ans_cnt, cname);

            ans_bytes += sub_ans_bytes;

            rewind(doc_file);

            //printf("cname: %s\n", cname);
            if(cname != NULL){
                uint8_t* compress_cname = (uint8_t*)calloc(strlen(cname)+1, sizeof(uint8_t));
                compress(cname, compress_cname);
                sub_ans_bytes = fillAnswer(res+header_bytes+ques_bytes+ans_bytes, doc_file, cname, compress_cname,\
                                    flag, ntohs(ques.QTYPE), &ans_cnt, NULL);
                
                ans_bytes += sub_ans_bytes;
                rewind(doc_file);
            }
        }

        auth_bytes = 0;
        printf("Filling Authority\n");
        uint8_t* auth_qname = ques.QNAME;
        auth_qname += strlen(domain) - strlen(query_domain);
        //for(int i = 0; i <= strlen(query_domain); i++)
            //printf("char %d: %u\n", i, *(auth_qname+i));
        
        if(ans_cnt == 0)
            auth_bytes = fillAuthority(res+header_bytes+ques_bytes, doc_file, auth_qname,\
                                        SOA, strlen(query_domain)+1, &auth_cnt);
        else if(ntohs(ques.QTYPE) != NS)
            auth_bytes = fillAuthority(res+header_bytes+ques_bytes+ans_bytes, doc_file, auth_qname,\
                                        NS, strlen(query_domain)+1, &auth_cnt);
        
        rewind(doc_file);
        printf("Filling Additional\n");
        add_bytes = 0;
        //printf("domain: %s\n", domain);

        memset(tmp_domain, 0, strlen(domain));
        tmp_domain = (char*)realloc(tmp_domain, strlen(domain)+6);
        uint8_t* add_qname;
        
        switch(ntohs(ques.QTYPE)){
            case MX:
                strcpy(tmp_domain, "mail.");
                strcpy(tmp_domain+5, domain);
                add_qname = (uint8_t*)calloc(strlen(tmp_domain)+1, sizeof(uint8_t));
                //printf("new domain: %s\n", tmp_domain);
                compress(tmp_domain, add_qname);
                break;
            case NS:
                strcpy(tmp_domain, "dns.");
                strcpy(tmp_domain+4, domain);
                //printf("new domain: %s\n", tmp_domain);
                add_qname = (uint8_t*)calloc(strlen(tmp_domain)+1, sizeof(uint8_t));
                compress(tmp_domain, add_qname);
                break;
            default:
                strcpy(tmp_domain, domain);
                break;
        }
        //strcpy(tmp_domain, domain);

        if(ans_cnt != 0 && strcmp(query_domain, domain) == 0)
            add_bytes = fillAddition(res+header_bytes+ques_bytes+ans_bytes+auth_bytes, doc_file, tmp_domain,\
                                    add_qname, &add_cnt);
        
        Header_t* h = (Header_t*)res;
        h->ANCOUNT = htons((uint16_t)ans_cnt);
        h->NSCOUNT = htons((uint16_t)auth_cnt);
        h->ARCOUNT = htons((uint16_t)add_cnt);

        fclose(doc_file);
        ShowHeader(h);
        if((slen = sendto(sockfd, res, header_bytes+ques_bytes+ans_bytes+auth_bytes+add_bytes, 0, (struct sockaddr*)&csin, csinlen)) < 0)
            printf("Send packet Problem\n");
    }
    

}