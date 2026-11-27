#include "PresetImport.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <charconv>
namespace fs=std::filesystem;
using namespace skellay;
const char* name(Model m){switch(m){case Model::classic:return "Classic (Delay 2)";case Model::modern:return "Modern (Delay 3)";case Model::bank:return "Bank (Delay Bank)";}return "Unknown";}
int main(int argc,char** argv){
 if(argc!=2&&argc!=4){std::cerr<<"Usage: skellay-preset INPUT.fst [INDEX OUTPUT.bin]\nIndexes start at 1. Inspect first, then select a delay.\n";return 2;}
 std::error_code ec;const auto size=fs::file_size(argv[1],ec);if(ec||size>64u*1024u*1024u){std::cerr<<"Unreadable or oversized input\n";return 2;}
 std::ifstream f(argv[1],std::ios::binary);std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));if(!f||!f.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(size))||f.peek()!=EOF){std::cerr<<"Cannot read complete input\n";return 2;}
 auto result=scanFlPreset(bytes);if(result.legacyDelaySeen)std::cerr<<"Original Fruity Delay (Delay 1) unsupported\n";if(result.rejected)std::cerr<<result.rejected<<" malformed or unsupported delay records skipped\n";
 if(result.entries.empty()){std::cerr<<result.error<<'\n';return 1;}
 for(std::size_t i=0;i<result.entries.size();++i){const auto& e=result.entries[i];std::cout<<i+1<<": "<<name(e.state.model)<<" track="<<e.track<<" slot="<<e.slot<<" bytes="<<e.state.bytes.size()<<'\n';}
 if(argc==2)return 0;
 unsigned index=0;std::string arg=argv[2];auto conversion=std::from_chars(arg.data(),arg.data()+arg.size(),index);if(conversion.ec!=std::errc{}||conversion.ptr!=arg.data()+arg.size()||!index||index>result.entries.size()){std::cerr<<"Invalid selection\n";return 2;}
 if(fs::exists(argv[3],ec)||ec){std::cerr<<"Refusing to overwrite existing output\n";return 2;}
 const auto& raw=result.entries[index-1].state.bytes;std::ofstream out(argv[3],std::ios::binary);if(!out.write(reinterpret_cast<const char*>(raw.data()),raw.size())||!out.flush()){std::cerr<<"Write failed\n";return 2;}std::cout<<"Saved exact raw parameter record\n";return 0;
}
