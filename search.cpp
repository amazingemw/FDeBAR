/* ##########
20171220 ZXARYO AND INFINITYDUDE @ MARS LAB, IITG
* This is not a part of booksim
* this file is to run a search 
* using grep

				Usage: ./search [OPTION]... PATTERN

				eg: ./search inT -i 
				yields results for string int in the files defined in string "content"
				
*/
#include<iostream>
#include<stdlib.h>
#include<string.h>
#include<fstream>
std::ifstream infile("search.txt");
using namespace std;

	char aa[]="grep ";
	string grep="grep "; 
	string content=" bgui.cpp bgui.hpp booksim_config.cpp booksim_config.hpp booksim.hpp booksim_manual.pdf buffer_state.cpp buffer_state.hpp channel.hpp configlex.cpp config.tab.c config_tab.cpp config.tab.h config_tab.hpp config_utils.cpp config_utils.hpp credit.cpp credit.hpp flitchannel.cpp flitchannel.hpp flit.cpp flit.hpp globals.hpp indexfile injection.cpp injection.hpp main.cpp minbd_script88 misc_utils.cpp misc_utils.hpp module.cpp module.hpp outputset.cpp outputset.hpp pipefifo.hpp power_module.cpp power_module.hpp random_utils.cpp random_utils.hpp rng.cpp rng_double.cpp rng_double_wrapper.cpp rng.hpp rng_wrapper.cpp routefunc.cpp routefunc.hpp stats.cpp stats.hpp traffic.cpp traffic.hpp trafficmanager.cpp trafficmanager.hpp vc.cpp vc.hpp routers/chaos_router.cpp routers/chaos_router.hpp routers/event_router.cpp routers/event_router.hpp routers/iq_router_base.cpp routers/iq_router_base.hpp routers/iq_router_baseline.cpp routers/iq_router_baseline.hpp routers/iq_router_combined.cpp routers/iq_router_combined.hpp routers/iq_router_split.cpp routers/iq_router_split.hpp routers/MECSChannels.cpp routers/MECSChannels.hpp routers/MECSCombiner.cpp routers/MECSCombiner.hpp routers/MECSCreditChannel.cpp routers/MECSCreditChannel.hpp routers/MECSCreditCombiner.cpp routers/MECSCreditCombiner.hpp routers/MECSCreditForwarder.cpp routers/MECSCreditForwarder.hpp routers/MECSForwarder.cpp routers/MECSForwarder.hpp routers/MECSRouter.cpp routers/MECSRouter.hpp routers/router.cpp routers/router.hpp  networks/anynet.cpp networks/anynet.hpp networks/cmesh.cpp networks/cmesh.hpp networks/cmeshx2.cpp networks/cmeshx2.hpp networks/cmo.cpp networks/cmo.hpp networks/dragonfly.cpp networks/dragonfly.hpp networks/fattree.cpp networks/fattree.hpp networks/flatfly_onchip.cpp networks/flatfly_onchip.hpp networks/fly.cpp networks/fly.hpp networks/inf.txt networks/isolated_mesh.cpp networks/isolated_mesh.hpp networks/kncube.cpp networks/kncube.hpp networks/mecs.cpp networks/mecs.hpp networks/network.cpp networks/network.hpp networks/qtree.cpp networks/qtree.hpp networks/singlenet.cpp networks/singlenet.hpp networks/tree4.cpp networks/tree4.hpp"; 
	char *ab=NULL;
int main(int argc, char **argv)
{

	//std::string content((std::istreambuf_iterator<char>(infile) ),
      //                  (std::istreambuf_iterator<char>()));
	//cout<<content;
	string s;
	for (int i = 1; i<argc; ++i)
		{
			s=s+argv[i];
			s=s+" ";
		}
	cout<<s;
	grep=grep+s+content;
	const char *aa=grep.c_str();
	system(aa);
	return(0);
}