// dhcpc : A DHCP client for Syllable
// (C)opyright 2002-2003 Kristian Van Der Vliet
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// Changes
//
// 1.0.1 : Vanders -:- Fixed a braindead-ism in get_mac_address() and setup_interface()
//

#include "interface.h"
#include "dhcp.h"
#include "debug.h"

#include <unistd.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <posix/errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/sockios.h>
#include <net/if.h>
#include <net/route.h>
#include <arpa/inet.h>
#include <netinet/in.h>

extern DHCPSessionInfo_s* info;

int get_mac_address( char* if_name )
{
	int if_socket;
	struct ifreq sReq;

	debug(INFO,__FUNCTION__,"looking up hardware address of interface %s\n",if_name);

	if_socket = socket(AF_INET,SOCK_STREAM,0);
	if( if_socket < 0 )
	{
		debug(PANIC,__FUNCTION__,"unable to create socket\n");
		return( EINVAL );
	}

	memset( &sReq, 0, sizeof( sReq ) );
	strcpy( sReq.ifr_name, if_name );

	info->if_hwaddress = malloc(6);
	if( info->if_hwaddress == NULL )
	{
		debug(PANIC,__FUNCTION__,"unable to malloc space for hardware address\n");
		close( if_socket );
		return( ENOMEM );
	}

	if( ioctl(if_socket, SIOCGIFHWADDR, &sReq) < 0 )
	{
		debug( PANIC,__FUNCTION__,"failed to read HW address for interface %s\n",if_name);
		close( if_socket );
		return( EINVAL );
	}

	info->if_hwaddress = memcpy( info->if_hwaddress, sReq.ifr_hwaddr.sa_data,6);

	close( if_socket );

	if( info->if_hwaddress == NULL )
	{
		debug(PANIC,__FUNCTION__,"could not find interface %s\n",if_name);
		return( EINVAL );
	}

	debug(INFO,__FUNCTION__,"MAC address for interface %s is %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", if_name, info->if_hwaddress[0], info->if_hwaddress[1], info->if_hwaddress[2], info->if_hwaddress[3], info->if_hwaddress[4], info->if_hwaddress[5]);

	return( EOK );
}

int bringup_interface( char* if_name )
{
	int if_socket;
	struct ifreq sIFReq;

	debug(INFO,__FUNCTION__,"checking interface %s\n",if_name);

	if_socket = socket(AF_INET,SOCK_STREAM,0);
	if( if_socket < 0 )
	{
		debug(PANIC,__FUNCTION__,"unable to create socket\n");
		return( EINVAL );
	}

	memset( &sIFReq, 0, sizeof( sIFReq ) );
	strcpy( sIFReq.ifr_name, if_name );

	if ( ioctl( if_socket, SIOCGIFFLAGS, &sIFReq ) < 0 )
	{
		debug( PANIC,__FUNCTION__,"failed to read flags for interface %s\n",if_name);
		close( if_socket );
		return( EINVAL );
	}

	if( sIFReq.ifr_flags & IFF_UP )
	{
		debug(INFO,__FUNCTION__,"interface %s is already up\n",if_name);
		close( if_socket );
		return( EOK );
	}

	debug(INFO,__FUNCTION__,"bringing up interface %s\n",if_name);
	sIFReq.ifr_flags |= IFF_UP;

	if ( ioctl( if_socket, SIOCSIFFLAGS, &sIFReq ) < 0 )
	{
		debug( PANIC,__FUNCTION__,"failed to set flags for interface %s\n",if_name);
		close( if_socket );
		return( EINVAL );
	}

	// Work around a bug where we must have a route on the interface before
	// we can broadcast a packet.  The IP, netmask and gateway address are
	// all 0.0.0.0 at this point.
	//
	// FIXME: Obviously, fix the bug & remove this code...
	if( setup_route( if_socket ) != EOK )
	{
		debug(WARNING,__FUNCTION__,"failed to set dummy route for interface %s\n",if_name);
		close( if_socket );
		return( EINVAL );
	}

	close( if_socket );
	return( EOK );
}

int setup_interface( void )
{
	int if_socket;
	struct ifreq sIFReq;
	char* ip;

	if_socket = socket(AF_INET,SOCK_STREAM,0);
	if( if_socket < 0 )
	{
		debug(PANIC,__FUNCTION__,"unable to create socket\n");
		return( EINVAL );
	}

	ip = format_ip( info->yiaddr );
	debug(INFO,__FUNCTION__,"setting if %s IP %s\n", info->if_name, ip);
	free(ip);

	memset( &sIFReq, 0, sizeof( sIFReq ) );
	strcpy( sIFReq.ifr_name, info->if_name );

	// Set the IP address & netmask
	memset( &sIFReq, 0, sizeof( sIFReq ) );
	strcpy( sIFReq.ifr_name, info->if_name );
	memcpy( &(((struct sockaddr_in*)&sIFReq.ifr_addr))->sin_addr, &info->yiaddr, 4 );

	if ( ioctl( if_socket, SIOCSIFADDR, &sIFReq ) < 0 )
	{
		ip = format_ip(info->yiaddr);

		debug(PANIC,__FUNCTION__,"failed to set interface %s address %s\n",info->if_name, ip);
		debug(PANIC,__FUNCTION__,"%s\n",strerror( errno ));

		close( if_socket );
		free(ip);
		return( EINVAL );
	}
    
	memset( &sIFReq, 0, sizeof( sIFReq ) );
	strcpy( sIFReq.ifr_name, info->if_name );
	memcpy( &(((struct sockaddr_in*)&sIFReq.ifr_addr))->sin_addr, &info->subnetmask, 4 );

	if ( ioctl( if_socket, SIOCSIFNETMASK, &sIFReq ) < 0 )
	{
		char* mask = format_ip(info->subnetmask);

		debug(PANIC,__FUNCTION__,"failed to set interface %s netmask %s\n",info->if_name, mask);
		debug(PANIC,__FUNCTION__,"%s\n",strerror( errno ));

		close( if_socket );
		free(mask);
		return( EINVAL );
	}

	// Add gateway routes, if we have any
	if( info->routers != NULL )
	{
		if( setup_route( if_socket ) != EOK )
			debug(PANIC,__FUNCTION__,"unable to configure gateway route\n");
	}
	else
		debug(INFO,__FUNCTION__,"no routes to configure\n");

	// We're done with the socket stuff
	close( if_socket );

	// Do none-socket stuff
	if( setup_resolver() != EOK )
	{
		debug(PANIC,__FUNCTION__,"unable configure resolv.conf entries\n");
		return( EINVAL );
	}

	return( EOK );
}

int setup_route( int if_socket )
{
	struct rtentry route;

	// Add gateway route

	memset( &route, 0, sizeof( route ) );
	route.rt_metric = 1;
	route.rt_flags |= RTF_GATEWAY;
	route.rt_dev = info->if_name;

	memcpy( &(((struct sockaddr_in*)&route.rt_dst))->sin_addr, &info->yiaddr, 4 );
	memcpy( &(((struct sockaddr_in*)&route.rt_genmask))->sin_addr, &info->subnetmask, 4 );
	memcpy( &(((struct sockaddr_in*)&route.rt_gateway))->sin_addr, info->routers, 4 );

	debug( INFO, __FUNCTION__, "adding route to if %s IP: %s Mask: %s, GW: %s\n", route.rt_dev,
	    format_ip( (((struct sockaddr_in*)&route.rt_dst))->sin_addr.s_addr ),
	    format_ip( (((struct sockaddr_in*)&route.rt_genmask))->sin_addr.s_addr ),
	    format_ip( (((struct sockaddr_in*)&route.rt_gateway))->sin_addr.s_addr ) );

	// Now add the route to the interface
	if ( ioctl( if_socket, SIOCADDRT, &route ) < 0 )
	{
		debug( PANIC, __FUNCTION__, "failed to add route");
		return( EINVAL );
	}

	return( EOK );
}

int setup_resolver( void )
{
	int current_server;

	// Set the DNS servers (If we have any)
	if( info->dns_server_count > 0 )
	{
		int resolv_fd;

		resolv_fd = open("/etc/resolv.conf", O_CREAT | O_WRONLY);
		if( resolv_fd < 0 )
		{
			debug(PANIC,__FUNCTION__,"unable to open /etc/resolv.conf for writing\n");
			return( EOK );
		}

		for( current_server=0; current_server < info->dns_server_count; current_server++)
		{
			char *ip, *entry;

			entry = (char*)malloc(28);
			ip = format_ip(info->dns_servers[current_server]);
			sprintf(entry,"nameserver %s",ip);
			
			debug(INFO,__FUNCTION__,"adding resolv.conf entry \"%s\"\n",entry);

			write(resolv_fd, (void*)entry, strlen(entry));
			write(resolv_fd, "\n", 1);

			free(ip);
			free(entry);
		}

		close(resolv_fd);
	}
	return( EOK );
}

char* format_ip(uint32 addr)
{
	uint8 octets[4];
	char* buffer;

	buffer = calloc(1,33);
	memcpy(octets,&addr,4);

	sprintf(buffer,"%.1i.%.1i.%.1i.%.1i",octets[0],octets[1],octets[2],octets[3]);
	return(buffer);
}

