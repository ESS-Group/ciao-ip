#include "IPv4_TCP_Socket.h"

namespace ipstack {

bool IPv4_TCP_Socket::listen(){
  while(TCP_Socket::isEstablished() == false){
    if(_listen() == false){ //add listen-socket to Demux
      return false;
    }
    TCP_Socket::listen();
      
    //Wait for SYN packet
    while(TCP_Socket::isListening()) {
      IPv4_Packet* packet = IPv4_Socket::read(); //non-blocking read
      if(packet != 0){
        //add remote ipv4 addr
        IPv4_Socket::set_dst_ipv4_addr(packet->get_src_ipaddr());
        if(IPv4_Socket::get_src_ipv4_addr() == 0){
          //no route (interface) to remote ipv4 address found => address is invalid
          TCP_Socket::free(packet);
          _listen(); //listen for next SYN packet (at Demux)
        }
        else{
          unsigned len = packet->get_total_len() - packet->get_ihl()*4;
          TCP_Segment* segment = (TCP_Segment*) packet->get_data();
          TCP_Socket::set_dport(segment->get_sport()); //set dport to remote's sport
          //bind this socket at Demux to ipv4 addresses and tcp ports
          if(bind() == false){
            //this connection (ipv4 addresses, tcp ports) is already used
            TCP_Socket::free(packet);
            _listen(); //listen for next SYN packet (at Demux)
          }
          else{
            TCP_Socket::setMTU(IPv4_Socket::getMTU());
            TCP_Socket::input(segment, len); //deliver SYN packet
          }
        }
      }
      else{
        TCP_Socket::input(0, 0);
      }
    }
    recv_loop();
    if(TCP_Socket::isEstablished() == false){
      unbind(); //remove connection at Demux
    }
  }
  //return if connection is established
  return true;
}

} //namespace ipstack 
