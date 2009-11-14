/* Nessus Attack Scripting Language 
 *
 * Copyright (C) 2002 - 2004 Tenable Network Security
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2,
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
 /*
  * This file contains all the functions which deal with the remote host :
  * which ports are open, what is its IP, what is our IP, what transport
  * is on the remote port, and so on...
  */

#include <arpa/inet.h> /* for inet_aton */
#include <netdb.h> /* for gethostbyaddr */
#include <netinet/in.h> /* for in_addr */
#include <string.h> /* for strlen */
#include <unistd.h> /* for gethostname */

#include "network.h" /* for socket_get_next_source_addr */
#include "plugutils.h" /* for plug_get_host_fqdn */
#include "resolve.h" /* for nn_resolve */
#include "system.h" /* for estrdup */

#include "nasl_tree.h"
#include "nasl_global_ctxt.h"
#include "nasl_func.h"
#include "nasl_var.h"
#include "nasl_lex_ctxt.h"
#include "nasl_debug.h"
#include "exec.h"  

#include "nasl_host.h"

extern int is_local_ip (struct in_addr addr);  /* pcap.c */
extern int islocalhost (struct in_addr *addr); /* pcap.c */
extern char* routethrough (struct in_addr *dest,
                           struct in_addr *source); /* pcap.c */
extern char *v6_routethrough (struct in6_addr *dest, struct in6_addr *source);
extern int v6_islocalhost(struct in6_addr *);

tree_cell * get_hostname(lex_ctxt * lexic)
{
 struct arglist *  script_infos = lexic->script_infos;
 char * hostname = (char*)plug_get_host_fqdn(script_infos);
 tree_cell * retc;

 if( hostname == NULL )
	 return NULL;

 retc = alloc_tree_cell(0, NULL);
 retc->type = CONST_STR;
 retc->size = strlen(hostname);
 retc->x.str_val = estrdup(hostname);
 return retc;
}

tree_cell * get_host_ip(lex_ctxt * lexic)
{
 struct arglist *  script_infos = lexic->script_infos;
 struct in6_addr * ip = plug_get_host_ip(script_infos);
 char * txt_ip;
 tree_cell * retc;
 struct in_addr inaddr;
 char name[512];

 if(ip == NULL) /* WTF ? */
 {
   return FAKE_CELL;
 }

 inaddr.s_addr = ip->s6_addr32[3];
 retc = alloc_tree_cell(0, NULL);
 retc->type = CONST_STR;
 if(IN6_IS_ADDR_V4MAPPED(ip)) {
   txt_ip = estrdup(inet_ntoa(inaddr));
 }
 else {
   txt_ip = estrdup(inet_ntop(AF_INET6, ip, name, sizeof(name) ));
 }
 retc->x.str_val = estrdup(txt_ip);
 retc->size = strlen(retc->x.str_val);

 return retc;
}

tree_cell * get_host_open_port(lex_ctxt * lexic)
{
 struct arglist *  script_infos = lexic->script_infos;
 unsigned int port = plug_get_host_open_port(script_infos);
 tree_cell * retc;

 retc = alloc_tree_cell(0, NULL);
 retc->type = CONST_INT;
 retc->x.i_val = port;

 return retc;
}

tree_cell * get_port_state(lex_ctxt * lexic)
{
 int open;
 struct arglist *  script_infos = lexic->script_infos;
 tree_cell * retc;
 int port;

 port = get_int_var_by_num(lexic, 0, -1);
 if(port < 0)
	 return FAKE_CELL;

 retc = alloc_tree_cell(0, NULL);
 retc->type = CONST_INT;
 open = host_get_port_state(script_infos, port);
 retc->x.i_val = open;
 return retc;
}


tree_cell * get_udp_port_state(lex_ctxt * lexic)
{
 int open;
 struct arglist *  script_infos = lexic->script_infos;
 tree_cell * retc;
 int port;

 port = get_int_var_by_num(lexic, 0, -1);
 if(port < 0)
	 return FAKE_CELL;

 retc = alloc_tree_cell(0, NULL);
 retc->type = CONST_INT;
 open = host_get_port_state_udp(script_infos, port);
 retc->x.i_val = open;
 return retc;
}


tree_cell * nasl_islocalhost(lex_ctxt * lexic)
{
  struct arglist * script_infos = lexic->script_infos;
  struct in6_addr * dst = plug_get_host_ip(script_infos);
  tree_cell * retc;
  struct in_addr inaddr;

  retc = alloc_tree_cell(0, NULL);
  retc->type = CONST_INT;
  inaddr.s_addr = dst->s6_addr32[3];
  retc->x.i_val = v6_islocalhost(dst);
  return retc;
}


tree_cell * nasl_islocalnet(lex_ctxt * lexic)
{
 struct arglist *  script_infos = lexic->script_infos;
 struct in6_addr * ip = plug_get_host_ip(script_infos);
 tree_cell * retc;
  struct in_addr inaddr;

 retc = alloc_tree_cell(0, NULL);
 retc->type = CONST_INT;
  inaddr.s_addr = ip->s6_addr32[3];
 retc->x.i_val = v6_is_local_ip(ip);
 return retc;
}


tree_cell * nasl_this_host(lex_ctxt * lexic)
{
  struct arglist * script_infos = lexic->script_infos;
  tree_cell * retc;
  struct in6_addr addr;
  char hostname[255];
  char * ret;
  struct in6_addr *  ia = plug_get_host_ip(script_infos);
  struct in_addr src;
  struct in6_addr in6addr;
  struct in_addr inaddr;
  struct in6_addr src6;

  retc = alloc_tree_cell(0, NULL);
  retc->type = CONST_DATA;

  if(IN6_IS_ADDR_V4MAPPED(ia))
    addr = socket_get_next_source_v4_addr(arg_get_value(script_infos, "globals"));
  else
    addr = socket_get_next_source_v6_addr(arg_get_value(script_infos, "globals"));
  if ( !IN6_ARE_ADDR_EQUAL(&addr, &in6addr_any) )
  {
    if(IN6_IS_ADDR_V4MAPPED(&addr))
    {
      inaddr.s_addr = addr.s6_addr32[3];
      retc->x.str_val = estrdup(inet_ntop(AF_INET, &inaddr,hostname, sizeof(hostname)));
    }
    else
      retc->x.str_val = estrdup(inet_ntop(AF_INET6, &addr,hostname, sizeof(hostname)));

    retc->size = strlen(retc->x.str_val);
    return retc;
  }


  if(ia)
  {
    if(v6_islocalhost(ia))
    {
      memcpy(&src6, ia, sizeof(struct in6_addr));
    }
    else
    {
      (void)v6_routethrough(ia, &src6);
    }

    if(!IN6_ARE_ADDR_EQUAL(&src, &in6addr_any))
    {
      char * ret;

      if(IN6_IS_ADDR_V4MAPPED(&src6))
      {
        inaddr.s_addr = src6.s6_addr32[3];
        ret = estrdup(inet_ntop(AF_INET, &inaddr,hostname, sizeof(hostname)));
      }
      else
        ret = estrdup(inet_ntop(AF_INET6, &src6,hostname, sizeof(hostname)));

      retc->x.str_val = ret;
      retc->size = strlen(ret);

      return retc;
    }

    hostname[sizeof(hostname) - 1] = '\0';
    gethostname(hostname, sizeof(hostname) - 1);
    nn_resolve(hostname, &in6addr);

    if(IN6_IS_ADDR_V4MAPPED(&in6addr))
    {
      inaddr.s_addr = in6addr.s6_addr32[3];
      ret = estrdup(inet_ntop(AF_INET, &inaddr,hostname, sizeof(hostname)));
    }
    else
      ret = estrdup(inet_ntop(AF_INET6, &in6addr,hostname, sizeof(hostname)));

    retc->x.str_val = ret;
    retc->size = strlen(ret);
  }
  return retc;
}


tree_cell * nasl_this_host_name(lex_ctxt * lexic)
{
 char * hostname;
 tree_cell * retc;
 
 retc = alloc_tree_cell(0, NULL);
 retc->type = CONST_DATA;
 
 hostname = emalloc(256);
 gethostname(hostname, 255);
 
 retc->x.str_val = hostname;
 retc->size = strlen(hostname);
 return retc;
}


tree_cell * get_port_transport(lex_ctxt * lexic)
{
 struct arglist * script_infos =  lexic->script_infos;
 tree_cell *retc;
 int port = get_int_var_by_num(lexic, 0, -1);

 if(port >= 0)
 {
   int trp = plug_get_port_transport(script_infos, port);
   retc = alloc_tree_cell(0, NULL);
   retc->type = CONST_INT;
   retc->x.i_val = trp;
   return retc;
 }
 return NULL;
}

tree_cell*
nasl_same_host(lex_ctxt* lexic)
{
  tree_cell		*retc;
  struct hostent	*h;
  char			*hn[2], **names[2];
  struct in_addr	ia, *a[2];
  int			i, j, n[2], names_nb[2], flag;
  int			cmp_hostname = get_int_local_var_by_name(lexic, "cmp_hostname", 0);

 if ( check_authenticated(lexic) < 0 ) return NULL;


  for (i = 0; i < 2; i ++)
    {
      hn[i] = get_str_var_by_num(lexic, i);
      if (hn[i] == NULL)
	{
	  nasl_perror(lexic, "same_host needs two parameters!\n");
	  return NULL;
	}
      if ( strlen(hn[i]) >= 256 ) 
       {
	  nasl_perror(lexic, "same_host(): Too long hostname !\n");
	  return NULL;
       }
    }
  for (i = 0; i < 2; i ++)
    {
      if (! inet_aton(hn[i], &ia))	/* Not an IP address */
	{
	  h = gethostbyname(hn[i]);
	  if (h == NULL)
	    {
	      nasl_perror(lexic, "same_host: %s does not resolve\n", hn[i]);
	      n[i] = 0;
	      if (cmp_hostname) 
		{
		  names_nb[i] = 1;
		  names[i] = emalloc(sizeof(char*));
		  names[i][0] = estrdup(hn[i]);
		}
	    }
	  else
	    {
	      for (names_nb[i] = 0; h->h_aliases[names_nb[i]] != NULL; names_nb[i]++)
		;
	      names_nb[i] ++;
	      names[i] = emalloc(sizeof(char*) * names_nb[i]);
	      names[i][0] = estrdup(h->h_name);
	      for (j = 1; j < names_nb[i]; j ++)
		names[i][j] = estrdup(h->h_aliases[j-1]);

	      /* Here, we should check that h_addrtype == AF_INET */
	      for (n[i] = 0; ((struct in_addr**) h->h_addr_list)[n[i]] != NULL; n[i] ++)
		;
	      a[i] = emalloc(h->h_length * n[i]);
	      for (j = 0; j < n[i]; j ++)
		a[i][j] = *((struct in_addr**) h->h_addr_list)[j];
	    }
	}
      else
	{
	  if (cmp_hostname)
	    h = gethostbyaddr((const char *)&ia, sizeof(ia), AF_INET);
	  else
	    h = NULL;
	  if (h == NULL)
	    {
	      a[i] = emalloc(sizeof(struct in_addr));
	      memcpy(a[i], &ia, sizeof(struct in_addr));
	      n[i] = 1;
	    }
	  else
	    {
	      for (names_nb[i] = 0; h->h_aliases[names_nb[i]] != NULL; names_nb[i]++)
		;
	      names_nb[i] ++;
	      names[i] = emalloc(sizeof(char*) * names_nb[i]);
	      names[i][0] = estrdup(h->h_name);
	      for (j = 1; j < names_nb[i]; j ++)
		names[i][j] = estrdup(h->h_aliases[j-1]);

	      /* Here, we should check that h_addrtype == AF_INET */
	      for (n[i] = 0; ((struct in_addr**) h->h_addr_list)[n[i]] != NULL; n[i] ++)
		;
	      a[i] = emalloc(h->h_length * n[i]);
	      for (j = 0; j < n[i]; j ++)
		a[i][j] = *((struct in_addr**) h->h_addr_list)[j];
	    }
	}
    }
#if 0
  fprintf(stderr, "N1=%d\tN2=%d\n", n[0], n[1]);
#endif
  flag = 0;
  for (i = 0; i < n[0] && ! flag; i ++)
    for (j = 0; j < n[1] && ! flag; j ++)
      if (a[0][i].s_addr == a[1][j].s_addr)
	{
	  flag = 1;
#if 0
	  fprintf(stderr, "%s == ", inet_ntoa(a[0][i]));
	  fprintf(stderr, "%s\n", inet_ntoa(a[1][j]));
#endif
	}
#if 0
      else
	{
	  fprintf(stderr, "%s != ", inet_ntoa(a[0][i]));
	  fprintf(stderr, "%s\n", inet_ntoa(a[1][j]));
	}
#endif

  if (cmp_hostname)
    for (i = 0; i < names_nb[0] && ! flag; i ++)
    for (j = 0; j < names_nb[1] && ! flag; j ++)
      if(strcmp(names[0][i], names[1][j]) == 0)
	{
#if 0
	  fprintf(stderr, "%s == %s\n", names[0][i], names[1][j]);
#endif
	  flag = 1;
	}
#if 0
      else
	fprintf(stderr, "%s != %s\n", names[0][i], names[1][j]);
#endif

  retc = alloc_typed_cell(CONST_INT);
  retc->x.i_val = flag;

  for (i = 0; i < 2; i ++)
    efree(&a[i]);
  if (cmp_hostname)
    {
      for (i = 0; i < 2; i ++)
	for (j = 0; j < names_nb[i]; j ++)
	  efree(&names[i][j]);
      efree(&names[i]);
    }
  return retc;
}
