#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <bits/stdc++.h>

#define TCP_PORT 8080

#define BIT_RATE1 4096
#define BIT_RATE2 2048
#define BIT_RATE3 1024
#define BIT_RATE4 512
#define BIT_RATE5 256

#define MAX_STATION_SIZE 300

typedef struct station_list{
    int station_number;
    char station_name[255];
    int multicast_address;
    int data_port;
    int info_port;
    int bit_rate;
    char video_filename[255];
    float video_duration;
} stats;

typedef struct information{
    int size;
    stats data[MAX_STATION_SIZE];    
} inf;

inf parse_input();

using namespace std;

class station{
    public:
        int station_number;
        string station_name;
        int multicast_address;
        int data_port;
        int info_port;
        int bit_rate;
        string video_filename;
        int video_duration;

    public:
        station(int station_number,string station_name, int multicast_address, int data_port,int info_port,int bit_rate) { 
        this->station_number=station_number;
        this->station_name=station_name;
        this->multicast_address=multicast_address;
        this->data_port=data_port;    
        this->info_port=info_port;
        this->bit_rate=bit_rate;
        this->video_duration = get_file_duration(station_number,station_name,bit_rate);
        this->video_filename = "videos/"+station_name+".mp4";

    }

    string to_str(){
        return to_string(this->station_number) + "|" + this->station_name + "|" + to_string(this->multicast_address) + "|" + to_string(this->data_port) + "|" + to_string(this->info_port) + "|" + to_string(this->bit_rate) ;//+ "|" + to_string(this->video_duration) + "|" + this->video_filename;
    }
    
    int get_file_duration(int station_number,string station_name,int bit_rate){
        
        string sys_call = "ffmpeg -i videos/" + station_name + ".mp4 2>&1 | grep Duration | cut -d ' ' -f 4 | sed s/,// > duration" + to_string(station_number) + ".txt";
        
        system(sys_call.c_str());

        FILE *media_file;
        FILE *duration_file;

        string file_name = "videos/" + station_name + ".mp4";
        string duration_file_name = "duration" + to_string(station_number) + ".txt";
        
        media_file = fopen(file_name.c_str(),"re");
        duration_file = fopen(duration_file_name.c_str(),"re");
        
        if(media_file==NULL)
        {
            printf("Could not open mediafile!!");
            exit(1);
        }
        if(duration_file==NULL)
        {
            printf("Could not open durationfile!!");
            exit(1);
        }

        fseek(media_file,0,SEEK_END);
        int file_size = ftell(media_file);
        

        char duration[100] = {0};
        fread(duration,1,100,duration_file);
        int hour=0,min=0,sec=0;
        sscanf(duration,"%d:%d:%d",&hour,&min,&sec);
        cout << hour << ":" << min << ":" << sec << "\n\n";
        sys_call = "rm " + duration_file_name;
        
        fclose(media_file);
        fclose(duration_file);
        int total_duration = ((file_size)/(3600*hour + 60*min + sec))/bit_rate;
        system(sys_call.c_str()); 
        return total_duration;
    }

};

map<int,bool> v;
vector<station> station_vec;

void initialize_map(){
    for(int i=1;i<=255;i++){
        v[i]=true;
    }
}


int get_free_number(){
    for(auto it:v){
        if(it.second) return it.first;
    }
    return -1;
}


int add_station(){
    
    int station_number = get_free_number();
    if(station_number!=-1){
        string addr = "127.0.0.1";
        const char *address = addr.c_str();
        int data_port = 7000+station_number;
        int info_port = 8010+station_number;
        station new_station(station_number,"station"+to_string(station_number),inet_addr(address),data_port,info_port,BIT_RATE1);
        station_vec.push_back(new_station);
        v[station_number] = false;
        return station_number;
    }
    return -1;
}

void remove_station(int station_number){
    v[station_number] = true;
    int ind=0;
    
    for(auto ind_station : station_vec) {
        if(ind_station.station_number==station_number) {
            station_vec.erase(station_vec.begin()+ind);
            break;
        }
        ind++;
    }
}


void* fetch_stations(void* input){
    
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(TCP_PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        inf input_data = parse_input();
        send(new_socket, &input_data, sizeof(inf), 0);
    }
    
}


void* send_data(void* input){
    int multiport,infoport,duration,station_number,multicast_address,bit_rate;
    string videofilename,station_name;
    
    multiport = ((station*)input)->data_port;
    infoport = ((station*)input)->info_port;
    videofilename = ((station*)input)->video_filename;
    duration = ((station*)input)->video_duration;
    station_name = ((station*)input)->station_name;
    station_number = ((station*)input)->station_number;
    multicast_address = ((station*)input)->multicast_address;
    bit_rate = ((station*)input)->bit_rate;
    

    clock_t start, mid, end;
    double execTime;


    /*----------------------    SOCKET MULTI-CAST   --------------------*/
    int multi_sockfd;
    struct sockaddr_in servaddr;

    if ((multi_sockfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("server : socket");
        exit(1);
    }

    memset((char *)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = multicast_address;
    servaddr.sin_port = htons(multiport);
    
    char buffer[bit_rate];
    int send_status;
    
    FILE *mediaFile;
    int filesize, packet_index, read_size, total_sent;
    packet_index = 1;
    
    mediaFile = fopen(videofilename.c_str(), "re");
    execTime = 0;
    start = clock();
    while (!feof(mediaFile)) {

        read_size = fread(buffer, 1, bit_rate, mediaFile);
        total_sent += read_size;
        // printf("Packet Size: = %d\n", read_size);

        if ((send_status = sendto(multi_sockfd, buffer, sizeof(buffer), 0,(struct sockaddr *)&servaddr,sizeof(servaddr))) == -1) {
            perror("sender: sendto");
            exit(1);
        }
        // printf("%d : Packet Number: %i Multicast_address : %d\n", multiport, packet_index,multicast_address);
        packet_index++;
        if (packet_index % duration == 0) {
            mid = clock();
            execTime = ((double)(mid - start)) / CLOCKS_PER_SEC;
            execTime = (0.9 - execTime);
            usleep((int)(execTime * 1000000));
        
            start = clock();
        }
    }
    memset(buffer, 0, sizeof(buffer));
    close(multi_sockfd);
    return NULL;
}

inf parse_input(){
    int data_size = station_vec.size();
    inf input_data;
    input_data.size = data_size;
    stats temp_struc;
    
    for (int i = 0; i < data_size; i++){

        temp_struc.bit_rate = station_vec[i].bit_rate;
        temp_struc.data_port = station_vec[i].data_port;
        strcpy(temp_struc.video_filename,station_vec[i].video_filename.c_str());
        temp_struc.video_duration = station_vec[i].video_duration;
        temp_struc.info_port = station_vec[i].info_port;
        temp_struc.multicast_address = station_vec[i].multicast_address;
        strcpy(temp_struc.station_name,station_vec[i].station_name.c_str());
        temp_struc.station_number = station_vec[i].station_number;
        input_data.data[i] = temp_struc;
    }
    
    return input_data;
}

int main(int argc, char *argv[]){
    
    initialize_map();

    pthread_t ptid;

    pthread_create(&ptid,NULL,fetch_stations,NULL);
    sleep(0.5);
    
    while(true)
    {
        cout << "\n1. Add Station \n2. Remove Station\n3. Exit\n\nEnter your choice: ";
        int input;
        cin >> input;   
        if(input==1){                    
            int stat_number = add_station();
            stat_number > 0 ? cout << "Station Added Successfully\n" : cout << "Cannot add the station!\n";
            int ind = -1;
            cout << stat_number << "\n";
            for(int i=0;i<station_vec.size();i++){
                if(stat_number==station_vec[i].station_number){
                    ind = i;
                    break;
                } 
            }
            cout << ind << endl;
            pthread_t udpid;
            pthread_create(&udpid,NULL,send_data,(void *)&station_vec[ind]);
            sleep(1);
        }
        else if(input==2) {                           
            cout << "Enter station number for removing: ";
            int remove_station_number;
            cin >> remove_station_number;
            remove_station(remove_station_number);
            cout << "Station removed\n";
        } 
        else exit(1);
    }

    pthread_exit(NULL);    
}