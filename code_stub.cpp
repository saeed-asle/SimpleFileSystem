#include <iostream>
#include <vector>
#include <map>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
using namespace std;
#define DISK_SIZE 512

char decToBinary(int n) {
    return static_cast<char>(n);
}
int binaryToDec(char c) {
    return static_cast<int>(c);
} 
class fsInode {
    int fileSize;
    int block_in_use;
    int directBlock1;
    int directBlock2;
    int directBlock3;
    int singleInDirect;
    int doubleInDirect;
    int block_size;
    int block_location;
    public:
    fsInode(int _block_size) {
        fileSize = 0; 
        block_in_use = 0; 
        block_size = _block_size;
        directBlock1 = -1;
        directBlock2 = -1;
        directBlock3 = -1;
        singleInDirect = -1;
        doubleInDirect = -1;
    }
    void setfile(int filesize){
        fileSize+=filesize;
    }
    void setblock_in_use(int blockused){
        block_in_use+=blockused;
    }
    void setdirectBlock1(int directBlock){
        directBlock1=directBlock;
    }
    void setdirectBlock2(int directBlock){
        directBlock2=directBlock;
    }
    void setdirectBlock3(int directBlock){
        directBlock3=directBlock;
    }
    void setsingleInDirect(int singleindirect){
        singleInDirect=singleindirect;
    }
    void setdoubleInDirect(int doubleindirect){
        doubleInDirect=doubleindirect;
    }
    int getfile(){
        return fileSize;
    }
    int getblock_in_use(){
        return block_in_use;
    }
    int getdirectBlock1(){
        return directBlock1;
    }
    int getdirectBlock2(){
        return directBlock2;
    }
    int getdirectBlock3(){
        return directBlock3;
    }
    int getsingleInDirect(){
        return singleInDirect;
    }
    int getdoubleInDirect(){
        return doubleInDirect;
    }
    ~fsInode() { 
    }
};
class FileDescriptor {
    pair<string, fsInode*> file;
    bool inUse;
    public:
    FileDescriptor(string FileName, fsInode* fsi) {
        file.first = FileName;
        file.second = fsi;
        inUse = true;
    }
    string getFileName() {
        return file.first;
    }
    void setFileName(string newname) {
        file.first=newname;
    }
    fsInode* getInode() {  
        return file.second;
    }////////////////////////////////////////////////////////
    int GetFileSize() {
        return file.second->getfile();
    }
    bool isInUse() { 
        return (inUse); 
    }
    void setInUse(bool _inUse) {
        inUse = _inUse ;
    }
};
#define DISK_SIM_FILE "DISK_SIM_FILE.txt"
class fsDisk {
    FILE *sim_disk_fd;
    bool is_formated;
    int blockSize;
    int BitVectorSize;
    int *BitVector;
    map<string, fsInode*>  MainDir ; 
    vector< FileDescriptor > OpenFileDescriptors;
    int emptydeskblocks;
    public:
    fsDisk() {
        sim_disk_fd = fopen( DISK_SIM_FILE , "w+" );
        assert(sim_disk_fd);
        for (int i=0; i < DISK_SIZE ; i++) {
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            ret_val = fwrite( "\0" ,  1 , 1, sim_disk_fd );
            assert(ret_val == 1);
        }
        fflush(sim_disk_fd);
    }
    void listAll() {
        int i = 0;    
        for ( auto it = begin (OpenFileDescriptors); it != end (OpenFileDescriptors); ++it) {
            cout << "index: " << i << ": FileName: " << it->getFileName() <<  " , isInUse: "
                << it->isInUse() << " file Size: " << it->GetFileSize() << endl;
            i++;
        }
        char bufy;
        cout << "Disk content: '" ;
        for (i=0; i < DISK_SIZE ; i++) {
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            ret_val = fread(  &bufy , 1 , 1, sim_disk_fd );
            if (isprint(bufy)) {
                cout << bufy;
            } 
            else {
                cout<<"\"";// Handle non-printable characters or skip them
                // Example: cout << "[" << static_cast<int>(bufy) << "]";
            }
                   
        }
        cout << "'" << endl;
    }
    int allocate_new_block(){//fine empty blocks
        for (int i = 0; i < BitVectorSize; i++) {
            if (BitVector[i] == 0) {
                BitVector[i] = 1;
                emptydeskblocks--;
                return i;
            }
        }
        return -1;
    }
    void fsFormat( int blockSize =4) {//format evryhting
        if(blockSize==1){
            cerr<<"ERR"<<endl;
            return ;
        }
        if (this->BitVector != nullptr) {
            delete[] this->BitVector;
        }
        this->BitVectorSize=DISK_SIZE/blockSize;
        this->BitVector=new int[this->BitVectorSize];
        this->blockSize=blockSize;
        this->emptydeskblocks=this->BitVectorSize;
        for(int i=0;i<BitVectorSize;i++){
            this->BitVector[i]=0;
        }
        this->MainDir.clear();
        this->OpenFileDescriptors.clear();
        is_formated=true;
        assert(sim_disk_fd);
        for (int i=0; i < DISK_SIZE ; i++) {
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            ret_val = fwrite( "\0" ,  1 , 1, sim_disk_fd );
            assert(ret_val == 1);
        }
        fflush(sim_disk_fd);
    }
    int CreateFile(string fileName) {//create the file after checking that everthing aillegal and opening it
        if(blockSize==1 || this->is_formated==false || this->emptydeskblocks==0){
            cerr<<"ERR"<<endl;
            return -1;
        }
        if(fileName.empty()==true){
            cerr<<"ERR"<<endl;
            return -1;
        }
        auto it = MainDir.find(fileName);
        if (it != MainDir.end()) {
            cerr<<"ERR1"<<endl;
            return -1;
        }
        fsInode *fs=new fsInode(this->blockSize);
        this->MainDir[fileName] = fs;
        for(int i=0;i<this->OpenFileDescriptors.size();i++){
            if(this->OpenFileDescriptors[i].getFileName().empty()==true ){
               this->OpenFileDescriptors[i].setFileName(fileName);
               this->OpenFileDescriptors[i].setInUse(true);
               return i; 
            }
            if(this->OpenFileDescriptors[i].isInUse()==false ){
               FileDescriptor *fdescriptor = new FileDescriptor(fileName, fs);
               this->OpenFileDescriptors[i]=*fdescriptor;
               delete fdescriptor;
               return i; 
            }
        }
        FileDescriptor *fdescriptor = new FileDescriptor(fileName, fs);
        this->OpenFileDescriptors.push_back(*fdescriptor);
        delete fdescriptor;
        return this->OpenFileDescriptors.size()-1;
    }
    int OpenFile(string FileName ) {//open the file after checking that everthing aillegal by checking in open file descrptor array and maindir
        if(blockSize==1 || this->is_formated==false ){
            cerr<<"ERR"<<endl;
            return -1;
        }
        for (int i = 0; i < this->OpenFileDescriptors.size(); i++) {
            if(this->OpenFileDescriptors[i].getFileName().compare(FileName)==0 ){
                if( this->OpenFileDescriptors[i].isInUse()==false){
                    this->OpenFileDescriptors[i].setInUse(true);
                    return i;
                }
                cerr<<"ERR"<<endl;
                return -1;
            }
        }
        auto it = MainDir.find(FileName);
        if (it != MainDir.end()) {
            for (int i = 0; i < this->OpenFileDescriptors.size(); i++) {
                if(this->OpenFileDescriptors[i].getFileName().size()==0 ){
                    FileDescriptor *fdescriptor = new FileDescriptor(it->first,it->second);
                   this->OpenFileDescriptors[i]=*fdescriptor;
                   delete fdescriptor;
                   return i;
                }
            } 
            FileDescriptor *fdescriptor = new FileDescriptor(it->first,it->second);
            this->OpenFileDescriptors.push_back(*fdescriptor);
            delete fdescriptor;
            return this->OpenFileDescriptors.size()-1;
        }
        cerr<<"ERR"<<endl;
        return -1;
    }
    string CloseFile(int fd) {//close file
        if(blockSize==1 || this->is_formated==false ){
            cerr<<"ERR"<<endl;
            return "-1";
        }
        if (fd >= OpenFileDescriptors.size() || OpenFileDescriptors[fd].isInUse()==false || fd<0 ){
            cerr<<"ERR"<<endl;
            return "-1";
        }
        this->OpenFileDescriptors[fd].setInUse(false);
        return this->OpenFileDescriptors[fd].getFileName();
    }
    int WriteToFile(int fd, char *buf, int len ) {//write the file after checking that everthing aillegal write to file 
        if(fd<0 || fd>=this->OpenFileDescriptors.size()){
            cerr<<"ERR"<<endl;
            return -1;
        }
        if(blockSize==1 || this->is_formated==false ){
            cerr<<"ERR"<<endl;
            return -1;
        }
        fsInode *fs=OpenFileDescriptors[fd].getInode();
        int size=( 3 + this->blockSize + this->blockSize* this->blockSize ) * this->blockSize;
        if(fs->getfile()==size && len!=0){
            cerr<<"ERR"<<endl;
            return -1;
        }
        if(fs->getfile()+len>size){//check if smaller from emptys
            len=size-fs->getfile();
        }
        int index=0;
        while(index<len){
            int block=fs->getfile()/this->blockSize;
            int offset=fs->getfile()%this->blockSize;
            int fill;
            if(this->emptydeskblocks==0){
                return 1;
            }
            if(offset>0 && index+this->blockSize-offset<=len){
                fill=this->blockSize-offset;
            }
            else if(
                index+this->blockSize<=len){fill=this->blockSize;
            }
            else{
                fill=(len-index)%this->blockSize;
            }
            if(block<1){
                if(fs->getdirectBlock1()==-1){
                    fs->setdirectBlock1(allocate_new_block());
                }
                fseek(sim_disk_fd, fs->getdirectBlock1()*this->blockSize+offset, SEEK_SET);
                } ////
            else if(block<2){
                if(fs->getdirectBlock2()==-1){
                    fs->setdirectBlock2(allocate_new_block());
                }
                fseek(sim_disk_fd, fs->getdirectBlock2()*this->blockSize+offset, SEEK_SET);
            }////       
            else if(block<3){
                if(fs->getdirectBlock3()==-1){
                    fs->setdirectBlock3(allocate_new_block());
                }
                fseek(sim_disk_fd, fs->getdirectBlock3()*this->blockSize+offset, SEEK_SET);
            }///// 
            else if(block<(3+this->blockSize)){
                if(fs->getsingleInDirect()==-1){ 
                    fs->setsingleInDirect(allocate_new_block());
                }
                char bufy;
                int i=(fs->getfile()-3*this->blockSize)/this->blockSize;
                fseek(sim_disk_fd ,fs->getsingleInDirect()*this->blockSize+i ,SEEK_SET );
                fread(&bufy ,1 ,1, sim_disk_fd );
                if(bufy==0){
                    bufy=decToBinary(allocate_new_block());
                    fseek(sim_disk_fd ,fs->getsingleInDirect()*this->blockSize+i ,SEEK_SET );
                    fwrite(&bufy,1,1 , sim_disk_fd);
                }/////
                fseek(sim_disk_fd ,binaryToDec(bufy)*this->blockSize+offset,SEEK_SET );
            }
            else if(block<(3+this->blockSize+this->blockSize*this->blockSize)){
                if(fs->getdoubleInDirect()==-1){
                    fs->setdoubleInDirect(allocate_new_block());
                }
                char bufy;char bufy1;
                int i=(fs->getfile()-(3+this->blockSize)*this->blockSize)/(this->blockSize*this->blockSize);
                int j=(fs->getfile()-(3+this->blockSize)*this->blockSize)%(this->blockSize*this->blockSize);
                int ij=j/this->blockSize;
                fseek(sim_disk_fd ,fs->getdoubleInDirect()*this->blockSize+i ,SEEK_SET );
                fread(&bufy ,1 ,1, sim_disk_fd );
                if(bufy==0){
                    bufy=decToBinary(allocate_new_block());
                    fseek(sim_disk_fd ,fs->getdoubleInDirect()*this->blockSize+i ,SEEK_SET );
                    fwrite(&bufy,1,1 , sim_disk_fd);
                }/////
                fseek(sim_disk_fd ,binaryToDec(bufy)*this->blockSize+ij ,SEEK_SET );
                fread(&bufy1 ,1 ,1, sim_disk_fd );
                if(bufy1==0){
                    bufy1=decToBinary(allocate_new_block());
                    fseek(sim_disk_fd ,binaryToDec(bufy)*this->blockSize+ij ,SEEK_SET );
                    fwrite(&bufy1,1,1 , sim_disk_fd);
                }/////
                fseek(sim_disk_fd ,binaryToDec(bufy1)*this->blockSize+offset,SEEK_SET );
            }
            fwrite(&buf[index],1,fill , sim_disk_fd);
            fflush(sim_disk_fd);
            index+=fill;
            fs->setfile(fill);
        }//blocluesds
        return 1;
    }
    int DelFile( string FileName ){//delete the file after checking that everthing aillegal and close them delete also the data and veribales
        if(blockSize==1 || this->is_formated==false){
            cerr<<"ERR"<<endl;
            return -1;
        }
        int found_index=-1;
        auto it = MainDir.find(FileName);
        if (it != MainDir.end()) {
            for (int i = 0; i < this->OpenFileDescriptors.size(); i++) {
                if(this->OpenFileDescriptors[i].getFileName().compare(FileName)==0){
                    if( this->OpenFileDescriptors[i].isInUse()==true){
                        cerr<<"ERR"<<endl;
                        return -1;
                    }
                    found_index=i;
                    break;
                }
            }
        }
        fsInode *fs=OpenFileDescriptors[found_index].getInode();
        int index=fs->getfile();int fill=0;
        char delete_char[index];
        for (int i = 0; i < index; i++){
            delete_char[i] = '\0';
        }
        
        while(index>=0){
            int block=index/this->blockSize;
            int offset=index%this->blockSize;
            if(offset>0 ){
                fill=offset;
            }
            else{
                fill=this->blockSize;
            }
            if(block<1){
                this->BitVector[fs->getdirectBlock1()]=0;
                fseek(sim_disk_fd, fs->getdirectBlock1()*this->blockSize, SEEK_SET);
            } 
            else if(block<2){
                this->BitVector[fs->getdirectBlock2()]=0;
                fseek(sim_disk_fd, fs->getdirectBlock2()*this->blockSize, SEEK_SET);
            }      
            else if(block<3){
                this->BitVector[fs->getdirectBlock3()]=0;
                fseek(sim_disk_fd, fs->getdirectBlock3()*this->blockSize, SEEK_SET);
            }
            else if(block<(3+this->blockSize)){
                char bufy;
                int i=(index-3*this->blockSize)/this->blockSize;
                this->BitVector[fs->getsingleInDirect()]=0;
                fseek(sim_disk_fd ,fs->getsingleInDirect()*this->blockSize+i ,SEEK_SET );
                fread(&bufy ,1 ,1, sim_disk_fd );
                fseek(sim_disk_fd ,fs->getsingleInDirect()*this->blockSize+i ,SEEK_SET );
                fwrite(&delete_char[index],1,1 , sim_disk_fd);
                this->BitVector[binaryToDec(bufy)]=0;
                fseek(sim_disk_fd ,binaryToDec(bufy)*this->blockSize,SEEK_SET);
            }
            else if(block<(3+this->blockSize+this->blockSize*this->blockSize)){
                char bufy;char bufy1;
                int i=(index-(3+this->blockSize)*this->blockSize)/(this->blockSize*this->blockSize);
                int j=(index-(3+this->blockSize)*this->blockSize)%(this->blockSize*this->blockSize);
                int ij=j/this->blockSize;
                this->BitVector[fs->getdoubleInDirect()]=0;
                fseek(sim_disk_fd ,fs->getdoubleInDirect()*this->blockSize+i ,SEEK_SET );
                fread(&bufy ,1 ,1, sim_disk_fd );
                fseek(sim_disk_fd ,binaryToDec(bufy)*this->blockSize+ij ,SEEK_SET );
                fread(&bufy1 ,1 ,1, sim_disk_fd );
                fseek(sim_disk_fd ,binaryToDec(bufy)*this->blockSize+ij ,SEEK_SET );
                fwrite(&delete_char[index],1,1 , sim_disk_fd);
                this->BitVector[binaryToDec(bufy)]=0;
                this->BitVector[binaryToDec(bufy1)]=0;
                fseek(sim_disk_fd ,binaryToDec(bufy1)*this->blockSize,SEEK_SET );
            }
            fwrite(&delete_char[index],1,fill , sim_disk_fd); 
            index-=fill;
        }//blocluesds
        fseek(sim_disk_fd ,fs->getdoubleInDirect()*this->blockSize ,SEEK_SET );
        fwrite(&delete_char[0],1,4 , sim_disk_fd);
        fflush(sim_disk_fd);
        fs->setfile(-1*fs->getfile());
        this->BitVector[fs->getdoubleInDirect()]=0;
        this->BitVector[fs->getsingleInDirect()]=0;
        fs->setdoubleInDirect(-1);
        fs->setsingleInDirect(-1);
        fs->setdirectBlock1(-1);
        fs->setdirectBlock2(-1);
        fs->setdirectBlock3(-1);
        this->MainDir.erase(it);
        emptydeskblocks=0;
        for(int i=0;i<BitVectorSize;i++){
            if(this->BitVector[i]==0){
                emptydeskblocks+=1;
            }
        }        
        this->OpenFileDescriptors[found_index].setFileName("");
        return 1;        
    }
    int ReadFromFile(int fd, char *buf, int len ) {//read the file after checking that everthing aillegal and appending to the array
        if(fd<0 || fd>=this->OpenFileDescriptors.size()){
            cerr<<"ERR"<<endl;
            return -1;
        }  
        if(blockSize==1 || this->is_formated==false ){
            cerr<<"ERR"<<endl;
            return -1;
        }
        if(this->OpenFileDescriptors[fd].GetFileSize()==0){
            buf[0]='\0';
            return 1;
        }
        fsInode *fs=OpenFileDescriptors[fd].getInode();
        int size=( 3 + this->blockSize + this->blockSize* this->blockSize ) * this->blockSize;
        if(len>size ){//check if smaller from empty
            len=fs->getfile();
        }
        int index=0;int fill=0;
        while(index<len){
            int block=index/this->blockSize;
            int offset=index%this->blockSize;
            if(offset>0 && index+this->blockSize-offset<=len){
                fill=this->blockSize-offset;
            }
            else if(index+this->blockSize<=len){
                fill=this->blockSize;
            }
            else{
                fill=(len-index)%this->blockSize;
            }
            if(block<1){
                fseek(sim_disk_fd, fs->getdirectBlock1()*this->blockSize+offset, SEEK_SET);
            } 
            else if(block<2){
                fseek(sim_disk_fd, fs->getdirectBlock2()*this->blockSize+offset, SEEK_SET);
            }////       
            else if(block<3){
                fseek(sim_disk_fd, fs->getdirectBlock3()*this->blockSize+offset, SEEK_SET);
            }///// 
            else if(block<(3+this->blockSize)){
                char bufy;
                int i=(index-3*this->blockSize)/this->blockSize;
                fseek(sim_disk_fd ,fs->getsingleInDirect()*this->blockSize+i ,SEEK_SET );
                fread(&bufy ,1 ,1, sim_disk_fd );
                fseek(sim_disk_fd ,binaryToDec(bufy)*this->blockSize+offset,SEEK_SET );
            }
            else if(block<(3+this->blockSize+this->blockSize*this->blockSize)){
                char bufy;char bufy1;
                int i=(index-(3+this->blockSize)*this->blockSize)/(this->blockSize*this->blockSize);
                int j=(index-(3+this->blockSize)*this->blockSize)%(this->blockSize*this->blockSize);
                int ij=j/this->blockSize;
                fseek(sim_disk_fd ,fs->getdoubleInDirect()*this->blockSize+i ,SEEK_SET );
                fread(&bufy ,1 ,1, sim_disk_fd );
                fseek(sim_disk_fd ,binaryToDec(bufy)*this->blockSize+ij ,SEEK_SET );
                fread(&bufy1 ,1 ,1, sim_disk_fd );
                fseek(sim_disk_fd ,binaryToDec(bufy1)*this->blockSize+offset,SEEK_SET );
            }
            fread(&buf[index], 1,fill,sim_disk_fd);  
            index+=fill;
        }//blocluesds
        buf[len]='\0';
        return 1;
    }
    int CopyFile(string srcFileName, string destFileName) {//copy the file after checking that everthing aillegal and close them
        if(blockSize==1 || this->is_formated==false || this->emptydeskblocks==0){
            cerr<<"ERR"<<endl;
            return -1;
        }
        int check=-1;
        if(srcFileName.compare(destFileName)==0){
            cerr<<"ERR"<<endl;
            return -1;  
        }
        if(this->emptydeskblocks==0){
            cerr<<"ERR"<<endl;
            return -1;
        }
        for (int i = 0; i < this->OpenFileDescriptors.size(); i++) {
            if(this->OpenFileDescriptors[i].getFileName().compare(srcFileName)==0){
                if(this->OpenFileDescriptors[i].isInUse()==true){
                    cerr<<"ERR"<<endl;
                    return -1;
                }
                check= i;
            }
        }
        for (int i = 0; i < this->OpenFileDescriptors.size(); i++) {
            if(this->OpenFileDescriptors[i].getFileName().compare(destFileName)==0){
                if(this->OpenFileDescriptors[i].isInUse()==true){
                    cerr<<"ERR"<<endl;
                    return -1;
                }
            }
        }
        bool right=false;
        bool right1=false;
        if(check==-1){
            check=OpenFile(srcFileName);
            right1=true;            
        }
        
        fsInode *fs=OpenFileDescriptors[check].getInode();
        if(ceil(fs->getfile()/blockSize)>=this->emptydeskblocks){
            cerr<<"ERR"<<endl;
            return -1;
        }
        if(ceil(fs->getfile()/blockSize)<=(3+blockSize)){
            if(ceil(fs->getfile()/blockSize)+(fs->getfile()-3*blockSize)/blockSize>=this->emptydeskblocks){
                cerr<<"ERR"<<endl;
                return -1;
            }
        }
        if(ceil(fs->getfile()/blockSize)<=(3+blockSize+blockSize*blockSize)){
            if(ceil(fs->getfile()/blockSize)+ceil(((fs->getfile()-3*blockSize)/(blockSize*blockSize))*blockSize)+blockSize>=this->emptydeskblocks){
                cerr<<"ERR"<<endl;
                return -1;
            }
        }
        char str_to_read[fs->getfile()];
        ReadFromFile( check , str_to_read ,fs->getfile() );
        auto it = MainDir.find(destFileName);
        if (it != MainDir.end()) {
            for (int i = 0; i < this->OpenFileDescriptors.size(); i++) {
                if(this->OpenFileDescriptors[i].getFileName().compare(destFileName)==0){
                    DelFile(destFileName);
                    right=true;
                }
            }
            if(right==false){
                CloseFile(OpenFile(destFileName));
                DelFile(destFileName);

            }
        }
        int newfile=CreateFile(destFileName);
        WriteToFile(newfile,str_to_read,fs->getfile());
        CloseFile(newfile);
        return 1;
    }
    int RenameFile(string oldFileName, string newFileName) {//renaming the file after checking it that all illegal
        if(blockSize==1 || this->is_formated==false){
            cerr<<"ERR"<<endl;
            return -1;
        }
        if(oldFileName.compare(newFileName)==0){
            cerr<<"ERR"<<endl;
            return -1;  
        }
        auto it = MainDir.find(newFileName);
        if (it != MainDir.end()) {
            cerr<<"ERR"<<endl;
            return -1; 
        }
        int check=-1;
        for (int i = 0; i < this->OpenFileDescriptors.size(); i++) {
            if(this->OpenFileDescriptors[i].getFileName().compare(oldFileName)==0){
                if(this->OpenFileDescriptors[i].isInUse()==true){
                    cerr<<"ERR"<<endl;
                    return -1;
                }
                check=i;
            }
        }
        it = MainDir.find(oldFileName);
        if (it != MainDir.end()) {
            if(check==-1){
                check=OpenFile(oldFileName);
            }
            MainDir.erase(it);
            MainDir[newFileName] = it->second;
            this->OpenFileDescriptors[check].setFileName(newFileName);
            this->OpenFileDescriptors[check].setInUse(false);
            return 1; // Success
        }
        cerr<<"ERR"<<endl;
        return -1;
    } 
    ~fsDisk() {//delete evrything
        for (int i = 0; i < OpenFileDescriptors.size(); i++) {
                delete OpenFileDescriptors[i].getInode();
        }
        if (BitVector != nullptr) {
            delete[] BitVector;
        }
        MainDir.clear();
        OpenFileDescriptors.clear();
        if (sim_disk_fd != nullptr) {
            fclose(sim_disk_fd);
        }
    }
};  
int main() {
    int blockSize; 
    string fileName;
    string fileName2;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read; 
    int _fd;

    fsDisk *fs = new fsDisk();
    int cmd_;
    while(1) {
        cin >> cmd_;
        switch (cmd_)
        {
            case 0:   // exit
				delete fs;
				exit(0);
                break;

            case 1:  // list-file
                fs->listAll(); 
                break;
          
            case 2:    // format
                cin >> blockSize;
                fs->fsFormat(blockSize);
                break;
          
            case 3:    // creat-file
                cin >> fileName;
                _fd = fs->CreateFile(fileName);
                cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
            
            case 4:  // open-file
                cin >> fileName;
                _fd = fs->OpenFile(fileName);
                cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
             
            case 5:  // close-file
                cin >> _fd;
                fileName = fs->CloseFile(_fd); 
                cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
           
            case 6:   // write-file
                cin >> _fd;
                cin >> str_to_write;
                fs->WriteToFile( _fd , str_to_write , strlen(str_to_write) );
                break;
          
            case 7:    // read-file
                cin >> _fd;
                cin >> size_to_read ;
                fs->ReadFromFile( _fd , str_to_read , size_to_read );
                cout << "ReadFromFile: " << str_to_read << endl;
                break;
           
            case 8:   // delete file 
                 cin >> fileName;
                _fd = fs->DelFile(fileName);
                cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 9:   // copy file
                cin >> fileName;
                cin >> fileName2;
                fs->CopyFile(fileName, fileName2);
                break;

            case 10:  // rename file
                cin >> fileName;
                cin >> fileName2;
                fs->RenameFile(fileName, fileName2);
                break;

            default:
                break;
        }
    }
}
 
