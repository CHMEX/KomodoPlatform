
/******************************************************************************
 * Copyright © 2014-2017 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/
//
//  LP_nativeDEX.c
//  marketmaker
//
// BCH signing: FORKID_BCC = 0, FORKID_BTG = 79, // Atomic number AU
// alice waiting for bestprice
// MNZ getcoin strangeness
// improve critical section detection when parallel trades
// reduce mem: dont redundant store pubkey utxo info
// previously, it used to show amount, kmd equiv, perc
// dPoW security -> 4: KMD notarized, 5: BTC notarized, after next notary elections
// bigendian architectures need to use little endian for sighash calcs

#include <stdio.h>

long LP_cjson_allocated,LP_cjson_total,LP_cjson_count;

struct LP_millistats
{
    double lastmilli,millisum,threshold;
    uint32_t count;
    char name[64];
} LP_psockloop_stats,LP_reserved_msgs_stats,utxosQ_loop_stats,command_rpcloop_stats,queue_loop_stats,prices_loop_stats,LP_coinsloop_stats,LP_coinsloopBTC_stats,LP_coinsloopKMD_stats,LP_pubkeysloop_stats,LP_privkeysloop_stats,LP_swapsloop_stats,LP_gcloop_stats;
extern int32_t IAMLP;

void LP_millistats_update(struct LP_millistats *mp)
{
    double elapsed,millis;
    if ( mp == 0 )
    {
        if ( IAMLP != 0 )
        {
            mp = &LP_psockloop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        }
        mp = &LP_reserved_msgs_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &utxosQ_loop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &command_rpcloop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &queue_loop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &prices_loop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &LP_coinsloop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &LP_coinsloopBTC_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &LP_coinsloopKMD_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &LP_pubkeysloop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &LP_privkeysloop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &LP_swapsloop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &LP_gcloop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
    }
    else
    {
        if ( mp->lastmilli == 0. )
            mp->lastmilli = OS_milliseconds();
        else
        {
            mp->count++;
            millis = OS_milliseconds();
            elapsed = (millis - mp->lastmilli);
            mp->millisum += elapsed;
            if ( mp->threshold != 0. && elapsed > mp->threshold )
            {
                //if ( IAMLP == 0 )
                    printf("%32s elapsed %10.2f millis > threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,elapsed,mp->threshold,mp->millisum/mp->count,mp->count);
            }
            mp->lastmilli = millis;
        }
    }
}

#include "LP_include.h"
portable_mutex_t LP_peermutex,LP_UTXOmutex,LP_utxomutex,LP_commandmutex,LP_cachemutex,LP_swaplistmutex,LP_forwardmutex,LP_pubkeymutex,LP_networkmutex,LP_psockmutex,LP_coinmutex,LP_messagemutex,LP_portfoliomutex,LP_electrummutex,LP_butxomutex,LP_reservedmutex,LP_nanorecvsmutex,LP_tradebotsmutex,LP_gcmutex,LP_inusemutex,LP_cJSONmutex;
int32_t LP_canbind;
char *Broadcaststr,*Reserved_msgs[2][1000];
int32_t num_Reserved_msgs[2],max_Reserved_msgs[2];
struct LP_peerinfo  *LP_peerinfos,*LP_mypeer;
struct LP_forwardinfo *LP_forwardinfos;
struct iguana_info *LP_coins;
struct LP_pubkeyinfo *LP_pubkeyinfos;
struct rpcrequest_info *LP_garbage_collector;
struct LP_address_utxo *LP_garbage_collector2;


//uint32_t LP_deadman_switch;
uint16_t LP_fixed_pairport,LP_publicport;
uint32_t LP_lastnonce,LP_counter,LP_swap_endcritical,LP_swap_critical;
int32_t LP_mybussock = -1;
int32_t LP_mypubsock = -1;
int32_t LP_mypullsock = -1;
int32_t LP_numfinished,LP_showwif,IAMLP = 0;
double LP_profitratio = 1.;

struct LP_privkey { bits256 privkey; uint8_t rmd160[20]; };

struct LP_globals
{
    struct LP_utxoinfo  *LP_utxoinfos[2],*LP_utxoinfos2[2];
    bits256 LP_mypub25519,LP_privkey,LP_mypriv25519;
    uint64_t LP_skipstatus[10000];
    uint8_t LP_myrmd160[20],LP_pubsecp[33];
    uint32_t LP_sessionid,counter;
    int32_t LP_IAMLP,LP_pendingswaps,USERPASS_COUNTER,LP_numprivkeys,initializing,waiting,LP_numskips;
    char USERPASS[65],USERPASS_WIFSTR[64],LP_myrmd160str[41],gui[16];
    struct LP_privkey LP_privkeys[100];
} G;

uint32_t LP_rand()
{
    uint32_t retval;
    retval = rand();
    retval = (retval << 7) ^ (retval >> 17) ^ rand();
    retval = (retval << 13) ^ (retval >> 13) ^ rand();
    retval = (retval << 17) ^ (retval >> 7) ^ rand();
    return(retval);
}

#include "LP_network.c"

char *activecoins[] = { "BTC", "KMD" };
char GLOBAL_DBDIR[] = { "DB" };
char LP_myipaddr[64],LP_publicaddr[64],USERHOME[512] = { "/root" };
char LP_gui[16] = { "cli" };

char *default_LPnodes[] = { "5.9.253.195", "5.9.253.196", "5.9.253.197", "5.9.253.198", "5.9.253.199", "5.9.253.200", "5.9.253.201", "5.9.253.202", "5.9.253.203",
    //"24.54.206.138", "173.212.225.176", "136.243.45.140", "107.72.162.127", "72.50.16.86", "51.15.202.191", "173.228.198.88",
    "51.15.203.171", "51.15.86.136", "51.15.94.249", "51.15.80.18", "51.15.91.40", "51.15.54.2", "51.15.86.31", "51.15.82.29", "51.15.89.155",
};//"5.9.253.204" }; //

// stubs

void tradebot_swap_balancingtrade(struct basilisk_swap *swap,int32_t iambob)
{
    
}

void tradebot_pendingadd(cJSON *tradejson,char *base,double basevolume,char *rel,double relvolume)
{
    // add to trades
}

char *LP_getdatadir()
{
    return(USERHOME);
}

char *blocktrail_listtransactions(char *symbol,char *coinaddr,int32_t num,int32_t skip)
{
    return(0);
}

#include "LP_socket.c"
#include "LP_secp.c"
#include "LP_bitcoin.c"
#include "LP_coins.c"
#include "LP_rpc.c"
#include "LP_cache.c"
#include "LP_RTmetrics.c"
#include "LP_utxo.c"
#include "LP_prices.c"
#include "LP_scan.c"
#include "LP_transaction.c"
#include "LP_stats.c"
#include "LP_remember.c"
#include "LP_swap.c"
#include "LP_peers.c"
#include "LP_utxos.c"
#include "LP_forwarding.c"
#include "LP_signatures.c"
#include "LP_ordermatch.c"
#include "LP_tradebots.c"
#include "LP_portfolio.c"
#include "LP_messages.c"
#include "LP_commands.c"

char *LP_command_process(void *ctx,char *myipaddr,int32_t pubsock,cJSON *argjson,uint8_t *data,int32_t datalen)
{
    char *retstr=0;
    if ( jobj(argjson,"result") != 0 || jobj(argjson,"error") != 0 )
        return(0);
    if ( LP_tradecommand(ctx,myipaddr,pubsock,argjson,data,datalen) <= 0 )
    {
        if ( (retstr= stats_JSON(ctx,myipaddr,pubsock,argjson,"127.0.0.1",0)) != 0 )
        {
            //printf("%s PULL.[%d]-> (%s)\n",myipaddr != 0 ? myipaddr : "127.0.0.1",datalen,retstr);
            //if ( pubsock >= 0 ) //strncmp("{\"error\":",retstr,strlen("{\"error\":")) != 0 &&
                //LP_send(pubsock,retstr,(int32_t)strlen(retstr)+1,0);
        }
    } //else printf("finished tradecommand (%s)\n",jprint(argjson,0));
    return(retstr);
}

char *LP_decrypt(uint8_t *ptr,int32_t *recvlenp)
{
    uint8_t decoded[LP_ENCRYPTED_MAXSIZE + crypto_box_ZEROBYTES],*nonce,*cipher; int32_t recvlen,cipherlen; char *jsonstr = 0;
    recvlen = *recvlenp;
    nonce = &ptr[2];
    cipher = &ptr[2 + crypto_box_NONCEBYTES];
    cipherlen = recvlen - (2 + crypto_box_NONCEBYTES);
    if ( cipherlen > 0 && cipherlen <= sizeof(decoded) )
    {
        if ( (jsonstr= (char *)_SuperNET_decipher(nonce,cipher,decoded,cipherlen,GENESIS_PUBKEY,G.LP_mypriv25519)) != 0 )
        {
            recvlen = (cipherlen - crypto_box_ZEROBYTES);
            if ( strlen(jsonstr)+1 != recvlen )
            {
                printf("unexpected len %d vs recvlen.%d\n",(int32_t)strlen(jsonstr)+1,recvlen);
                jsonstr = 0;
            } //else printf("decrypted (%s)\n",jsonstr);
        }
    } else printf("cipher.%d too big for %d\n",cipherlen,(int32_t)sizeof(decoded));
    *recvlenp = recvlen;
    return(jsonstr);
}

char *LP_process_message(void *ctx,char *typestr,char *myipaddr,int32_t pubsock,uint8_t *ptr,int32_t recvlen,int32_t recvsock)
{
    static uint32_t dup,uniq;
    int32_t i,len,cipherlen,datalen=0,duplicate=0,encrypted=0; char *method,*method2,*tmp,*cipherstr,*retstr=0,*jsonstr=0; cJSON *argjson; uint32_t crc32;
    //double millis = OS_milliseconds();
    crc32 = calc_crc32(0,&ptr[2],recvlen-2);
    if ( (crc32 & 0xff) == ptr[0] && ((crc32>>8) & 0xff) == ptr[1] )
        encrypted = 1;
    i = LP_crc32find(&duplicate,-1,crc32);
    if ( duplicate != 0 )
        dup++;
    else uniq++;
    portable_mutex_lock(&LP_commandmutex);
    if ( (LP_rand() % 10000) == 0 )
        printf("%s dup.%d (%u / %u) %.1f%% encrypted.%d recv.%u [%02x %02x] vs %02x %02x\n",typestr,duplicate,dup,dup+uniq,(double)100*dup/(dup+uniq),encrypted,crc32,ptr[0],ptr[1],crc32&0xff,(crc32>>8)&0xff);
    if ( duplicate == 0 )
    {
        if ( i >= 0 )
            LP_crc32find(&duplicate,i,crc32);
        if ( encrypted != 0 )
            jsonstr = LP_decrypt(ptr,&recvlen);
        else if ( (datalen= is_hexstr((char *)ptr,0)) > 0 )
        {
            datalen >>= 1;
            jsonstr = malloc(datalen + 1);
            decode_hex((void *)jsonstr,datalen,(char *)ptr);
            jsonstr[datalen] = 0;
        } else jsonstr = (char *)ptr;
        if ( jsonstr != 0 && (argjson= cJSON_Parse(jsonstr)) != 0 )
        {
            uint8_t decoded[LP_ENCRYPTED_MAXSIZE + crypto_box_ZEROBYTES];
            //printf("[%s]\n",jsonstr);
            cipherlen = 0;
            if ( (cipherstr= jstr(argjson,"cipher")) != 0 && (cipherlen= is_hexstr(cipherstr,0)) > 32 && cipherlen <= sizeof(decoded)*2 )
            {
                method2 = jstr(argjson,"method2");
                if ( (method= jstr(argjson,"method")) != 0 && (strcmp(method,"encrypted") == 0 ||(method2 != 0 && strcmp(method2,"encrypted") == 0)) )
                {
                    cipherlen >>= 1;
                    decode_hex(decoded,cipherlen,cipherstr);
                    crc32 = calc_crc32(0,&decoded[2],cipherlen-2);
                    if ( (tmp= LP_decrypt(decoded,&cipherlen)) != 0 )
                    {
                        jsonstr = tmp;
                        free_json(argjson);
                        argjson = cJSON_Parse(jsonstr);
                        recvlen = cipherlen;
                        encrypted = 1;
                        if ( (crc32 & 0xff) == decoded[0] && ((crc32>>8) & 0xff) == decoded[1] )
                        {
                            i = LP_crc32find(&duplicate,-1,crc32);
                            if ( duplicate == 0 && i >= 0 )
                                LP_crc32find(&duplicate,i,crc32);
                        }
                        printf("%02x %02x %08x duplicate.%d decrypted.(%s)\n",decoded[0],decoded[1],crc32,duplicate,jsonstr);
                    }
                    else
                    {
                        //printf("packet not for this node %u\n",crc32);
                    }
                } else printf("error (%s) method is %s\n",jsonstr,method);
            }
            if ( jsonstr != 0 && argjson != 0 )
            {
                len = (int32_t)strlen(jsonstr) + 1;
                if ( (method= jstr(argjson,"method")) != 0 && strcmp(method,"broadcast") == 0 )
                {
                    bits256 zero; cJSON *reqjson; char *cipherstr; int32_t cipherlen; uint8_t cipher[LP_ENCRYPTED_MAXSIZE];
                    if ( (reqjson= LP_dereference(argjson,"broadcast")) != 0 )
                    {
                        Broadcaststr = jprint(reqjson,0);
                        if ( (cipherstr= jstr(reqjson,"cipher")) != 0 )
                        {
                            cipherlen = (int32_t)strlen(cipherstr) >> 1;
                            if ( cipherlen <= sizeof(cipher) )
                            {
                                decode_hex(cipher,cipherlen,cipherstr);
                                LP_queuesend(calc_crc32(0,&cipher[2],cipherlen-2),LP_mypubsock,"","",cipher,cipherlen);
                            } else retstr = clonestr("{\"error\":\"cipher too big\"}");
                        }
                        else
                        {
                            memset(zero.bytes,0,sizeof(zero));
                            /*if ( (method= jstr(reqjson,"method")) != 0 && (strcmp(method,"request") == 0 || strcmp(method,"requested") == 0 || strcmp(method,"connect") == 0 || strcmp(method,"connected") == 0) )
                                    printf("broadcast.(%s)\n",Broadcaststr);*/
                            LP_reserved_msg(0,"","",zero,jprint(reqjson,0));
                        }
                        retstr = clonestr("{\"result\":\"success\"}");
                        free_json(reqjson);
                    } else retstr = clonestr("{\"error\":\"couldnt dereference sendmessage\"}");
                }
                else
                {
                    if ( (retstr= LP_command_process(ctx,myipaddr,pubsock,argjson,&((uint8_t *)ptr)[len],recvlen - len)) != 0 )
                    {
                    }
                }
            }
            if ( argjson != 0 )
                free_json(argjson);
        }
    } //else printf("DUPLICATE.(%s)\n",(char *)ptr);
    portable_mutex_unlock(&LP_commandmutex);
    if ( jsonstr != 0 && (void *)jsonstr != (void *)ptr && encrypted == 0 )
        free(jsonstr);
    return(retstr);
}

int32_t LP_sock_check(char *typestr,void *ctx,char *myipaddr,int32_t pubsock,int32_t sock,char *remoteaddr,int32_t maxdepth)
{
    int32_t recvlen=1,nonz = 0; cJSON *argjson; void *ptr; char methodstr[64],*retstr,*str; struct nn_pollfd pfd;
    if ( sock >= 0 )
    {
        while ( nonz < maxdepth && recvlen > 0 )
        {
            nonz++;
            memset(&pfd,0,sizeof(pfd));
            pfd.fd = sock;
            pfd.events = NN_POLLIN;
            if ( nn_poll(&pfd,1,1) != 1 )
                break;
            ptr = 0;
            //buf = malloc(1000000);
            //if ( (recvlen= nn_recv(sock,buf,1000000,0)) > 0 )
            if ( (recvlen= nn_recv(sock,&ptr,NN_MSG,0)) > 0 )
            {
                //ptr = buf;
                methodstr[0] = 0;
                //printf("%s.(%s)\n",typestr,(char *)ptr);
                if ( 0 )
                {
                    cJSON *recvjson; char *mstr;//,*cstr;
                    if ( (recvjson= cJSON_Parse((char *)ptr)) != 0 )
                    {
                        if ( (mstr= jstr(recvjson,"method")) != 0 )//&& strcmp(mstr,"uitem") == 0 && (cstr= jstr(recvjson,"coin")) != 0 && strcmp(cstr,"REVS") == 0 )
                        {
                            //printf("%s RECV.(%s)\n",typestr,(char *)ptr);
                        }
                        safecopy(methodstr,jstr(recvjson,"method"),sizeof(methodstr));
                        free_json(recvjson);
                    }
                }
                int32_t validreq = 0;
                if ( strlen((char *)ptr)+sizeof(bits256) <= recvlen )
                {
                    if ( LP_magic_check(ptr,recvlen,remoteaddr) <= 0 )
                    {
                        //printf("magic check error\n");
                    } else validreq = 1;
                    recvlen -= sizeof(bits256);
                }
                if ( validreq != 0 )
                {
                    if ( (retstr= LP_process_message(ctx,typestr,myipaddr,pubsock,ptr,recvlen,sock)) != 0 )
                        free(retstr);
                    if ( Broadcaststr != 0 )
                    {
                        //printf("self broadcast.(%s)\n",Broadcaststr);
                        str = Broadcaststr;
                        Broadcaststr = 0;
                        if ( (argjson= cJSON_Parse(str)) != 0 )
                        {
                            portable_mutex_lock(&LP_commandmutex);
                            if ( LP_tradecommand(ctx,myipaddr,pubsock,argjson,0,0) <= 0 )
                            {
                                if ( (retstr= stats_JSON(ctx,myipaddr,pubsock,argjson,remoteaddr,0)) != 0 )
                                    free(retstr);
                            }
                            portable_mutex_unlock(&LP_commandmutex);
                            free_json(argjson);
                        }
                        free(str);
                    }
                }
            }
            if ( ptr != 0 )
            {
                nn_freemsg(ptr), ptr = 0;
                //free(buf);
            }
        }
    }
    return(nonz);
}

int32_t LP_nanomsg_recvs(void *ctx)
{
    int32_t nonz = 0; char *origipaddr; struct LP_peerinfo *peer,*tmp;
    if ( (origipaddr= LP_myipaddr) == 0 )
        origipaddr = "127.0.0.1";
    //portable_mutex_lock(&LP_nanorecvsmutex);
    HASH_ITER(hh,LP_peerinfos,peer,tmp)
    {
        if ( peer->errors >= LP_MAXPEER_ERRORS )
        {
            if ( (LP_rand() % 10000) == 0 )
                peer->errors--;
            else
            {
                //printf("skip %s\n",peer->ipaddr);
                continue;
            }
        }
        //printf("check %s pubsock.%d\n",peer->ipaddr,peer->subsock);
        nonz += LP_sock_check("PULL",ctx,origipaddr,LP_mypubsock,peer->subsock,peer->ipaddr,1);
    }
    /*HASH_ITER(hh,LP_coins,coin,ctmp) // firstrefht,firstscanht,lastscanht
     {
     if ( coin->inactive != 0 )
     continue;
     if ( coin->bussock >= 0 )
     nonz += LP_sock_check(coin->symbol,ctx,origipaddr,-1,coin->bussock,LP_profitratio - 1.);
     }*/
    if ( LP_mypullsock >= 0 )
    {
        nonz += LP_sock_check("SUB",ctx,origipaddr,-1,LP_mypullsock,"127.0.0.1",1);
    }
    //portable_mutex_unlock(&LP_nanorecvsmutex);
    return(nonz);
}

void command_rpcloop(void *myipaddr)
{
    int32_t nonz = 0; void *ctx;
    ctx = bitcoin_ctx();
    strcpy(command_rpcloop_stats.name,"command_rpcloop");
    command_rpcloop_stats.threshold = 1000.;
    while ( 1 )
    {
        LP_millistats_update(&command_rpcloop_stats);
        nonz = LP_nanomsg_recvs(ctx);
        //if ( LP_mybussock >= 0 )
        //    nonz += LP_sock_check("BUS",ctx,origipaddr,-1,LP_mybussock);
        if ( nonz == 0 )
        {
            if ( IAMLP != 0 )
                usleep(10000);
            else usleep(50000);
        }
        else if ( IAMLP == 0 )
            usleep(1000);
    }
}

void utxosQ_loop(void *myipaddr)
{
    strcpy(utxosQ_loop_stats.name,"utxosQ_loop");
    utxosQ_loop_stats.threshold = 5000.;
    while ( 1 )
    {
        LP_millistats_update(&utxosQ_loop_stats);
        if ( LP_utxosQ_process() == 0 )
            usleep(50000);
    }
}

void LP_coinsloop(void *_coins)
{
    struct LP_address *ap=0,*atmp; struct LP_transaction *tx; cJSON *retjson; struct LP_address_utxo *up,*tmp; struct iguana_info *coin,*ctmp; char str[65]; struct electrum_info *ep,*backupep=0; bits256 zero; int32_t oldht,j,nonz; char *coins = _coins;
    if ( strcmp("BTC",coins) == 0 )
    {
        strcpy(LP_coinsloopBTC_stats.name,"BTC coin loop");
        LP_coinsloopBTC_stats.threshold = 20000.;
    }
    else if ( strcmp("KMD",coins) == 0 )
    {
        strcpy(LP_coinsloopKMD_stats.name,"KMD coin loop");
        LP_coinsloopKMD_stats.threshold = 10000.;
    }
    else
    {
        strcpy(LP_coinsloop_stats.name,"other coins loop");
        LP_coinsloop_stats.threshold = 5000.;
    }
    while ( 1 )
    {
        if ( strcmp("BTC",coins) == 0 )
            LP_millistats_update(&LP_coinsloopBTC_stats);
        else if ( strcmp("KMD",coins) == 0 )
            LP_millistats_update(&LP_coinsloopKMD_stats);
        else LP_millistats_update(&LP_coinsloop_stats);
        nonz = 0;
        HASH_ITER(hh,LP_coins,coin,ctmp) // firstrefht,firstscanht,lastscanht
        {
            if ( coins != 0 )
            {
                if ( coins[0] != 0 )
                {
                    if ( strcmp(coins,coin->symbol) != 0 )
                        continue;
                }
                else // avoid hardcode special case LP_coinsloop
                {
                    if ( strcmp("BTC",coin->symbol) == 0 || strcmp("KMD",coin->symbol) == 0 )
                        continue;
                }
            }
            if ( coin->smartaddr[0] == 0 )
                printf("%s has no smartaddress??\n",coin->symbol);
            memset(&zero,0,sizeof(zero));
            if ( coin->inactive != 0 )
                continue;
            if ( coin->longestchain == 1 ) // special init value
                coin->longestchain = LP_getheight(coin);
            if ( (ep= coin->electrum) != 0 )
            {
                if ( (backupep= ep->prev) == 0 )
                    backupep = ep;
                if ( (ap= LP_addressfind(coin,coin->smartaddr)) != 0 )
                {
                    if ( (retjson= electrum_address_listunspent(coin->symbol,ep,&retjson,ap->coinaddr,1)) != 0 )
                        free_json(retjson);
                }
                HASH_ITER(hh,coin->addresses,ap,atmp)
                {
                    //printf("call unspent %s\n",ap->coinaddr);
                    if ( (retjson= electrum_address_listunspent(coin->symbol,ep,&retjson,ap->coinaddr,1)) != 0 )
                        free_json(retjson);
                }
                if ( (ap= LP_addressfind(coin,coin->smartaddr)) != 0 )
                {
                    DL_FOREACH_SAFE(ap->utxos,up,tmp)
                    {
                        if ( up->U.height > 0 && up->spendheight < 0 )
                        {
                            if ( up->SPV == 0 )
                            {
                                nonz++;
                                up->SPV = LP_merkleproof(coin,coin->smartaddr,backupep,up->U.txid,up->U.height);
                                if ( up->SPV > 0 )
                                {
                                    if ( (tx= LP_transactionfind(coin,up->U.txid)) != 0 && tx->SPV == 0 )
                                    {
                                        tx->SPV = up->SPV;
                                        //printf("%s %s: SPV.%d\n",coin->symbol,bits256_str(str,up->U.txid),up->SPV);
                                    }
                                }
                            }
                            else if ( up->SPV == -1 )
                            {
                                nonz++;
                                printf("SPV failure for %s %s\n",coin->symbol,bits256_str(str,up->U.txid));
                                oldht = up->U.height;
                                LP_txheight_check(coin,ap->coinaddr,up);
                                if ( oldht != up->U.height )
                                    up->SPV = LP_merkleproof(coin,coin->smartaddr,backupep,up->U.txid,up->U.height);
                                if ( up->SPV <= 0 )
                                    up->SPV = -2;
                                else printf("%s %s: corrected SPV.%d\n",coin->symbol,bits256_str(str,up->U.txid),up->SPV);
                            }
                        }
                    }
                }
                while ( ep != 0 )
                {
                    if ( time(NULL) > ep->keepalive+LP_ELECTRUM_KEEPALIVE )
                    {
                        //printf("%s electrum.%p needs a keepalive: lag.%d\n",ep->symbol,ep,(int32_t)(time(NULL) - ep->keepalive));
                        if ( (retjson= electrum_address_listunspent(coin->symbol,ep,&retjson,coin->smartaddr,1)) != 0 )
                            free_json(retjson);
                    }
                    ep = ep->prev;
                }
                continue;
            }
            if ( coin->firstrefht == 0 )
                continue;
            else if ( coin->firstscanht == 0 )
                coin->lastscanht = coin->firstscanht = coin->firstrefht;
            else if ( coin->firstrefht < coin->firstscanht )
            {
                printf("detected %s firstrefht.%d < firstscanht.%d\n",coin->symbol,coin->firstrefht,coin->firstscanht);
                coin->lastscanht = coin->firstscanht = coin->firstrefht;
            }
            if ( coin->lastscanht == coin->longestchain+1 )
            {
                //printf("%s lastscanht.%d is longest.%d + 1\n",coin->symbol,coin->lastscanht,coin->longestchain);
                continue;
            }
            else if ( coin->lastscanht > coin->longestchain+1 )
            {
                printf("detected chain rewind lastscanht.%d vs longestchain.%d, first.%d ref.%d\n",coin->lastscanht,coin->longestchain,coin->firstscanht,coin->firstrefht);
                LP_undospends(coin,coin->longestchain-1);
                //LP_mempoolscan(coin->symbol,zero);
                coin->lastscanht = coin->longestchain - 1;
                if ( coin->firstscanht < coin->lastscanht )
                    coin->lastscanht = coin->firstscanht;
                continue;
            }
            nonz++;
            if ( 0 && coin->lastscanht < coin->longestchain-3 )
                printf("[%s]: %s ref.%d scan.%d to %d, longest.%d\n",coins,coin->symbol,coin->firstrefht,coin->firstscanht,coin->lastscanht,coin->longestchain);
            for (j=0; j<100; j++)
            {
                if ( LP_blockinit(coin,coin->lastscanht) < 0 )
                {
                    static uint32_t counter;
                    if ( counter++ < 3 )
                        printf("blockinit.%s %d error\n",coin->symbol,coin->lastscanht);
                    break;
                }
                coin->lastscanht++;
                if ( coin->lastscanht == coin->longestchain+1 )
                    break;
            }
        }
        if ( coins == 0 )
            return;
        if ( nonz == 0 )
            usleep(100000);
    }
}

int32_t LP_mainloop_iter(void *ctx,char *myipaddr,struct LP_peerinfo *mypeer,int32_t pubsock,char *pushaddr,uint16_t myport)
{
    static uint32_t counter;
    struct iguana_info *coin,*ctmp; char *origipaddr; uint32_t now; int32_t height,nonz = 0;
    if ( (origipaddr= myipaddr) == 0 )
        origipaddr = "127.0.0.1";
    if ( mypeer == 0 )
        myipaddr = "127.0.0.1";
    HASH_ITER(hh,LP_coins,coin,ctmp) // firstrefht,firstscanht,lastscanht
    {
        now = (uint32_t)time(NULL);
        if ( (coin->addr_listunspent_requested != 0 && now > coin->lastpushtime+LP_ORDERBOOK_DURATION*.5) || now > coin->lastpushtime+LP_ORDERBOOK_DURATION*5 )
        {
            //printf("PUSH addr_listunspent_requested %u\n",coin->addr_listunspent_requested);
            coin->lastpushtime = (uint32_t)now;
            LP_smartutxos_push(coin);
            coin->addr_listunspent_requested = 0;
        }
        if ( coin->electrum == 0 && coin->inactive == 0 && now > coin->lastgetinfo+LP_GETINFO_INCR )
        {
            nonz++;
            if ( (height= LP_getheight(coin)) > coin->longestchain )
            {
                coin->longestchain = height;
                if ( 0 && coin->firstrefht != 0 )
                    printf(">>>>>>>>>> set %s longestchain %d (ref.%d [%d, %d])\n",coin->symbol,height,coin->firstrefht,coin->firstscanht,coin->lastscanht);
            } //else LP_mempoolscan(coin->symbol,zero);
            coin->lastgetinfo = (uint32_t)now;
        }
    }
    counter++;
    return(nonz);
}

void LP_initcoins(void *ctx,int32_t pubsock,cJSON *coins)
{
    int32_t i,n; cJSON *item; char *symbol; struct iguana_info *coin;
    for (i=0; i<sizeof(activecoins)/sizeof(*activecoins); i++)
    {
        printf("%s ",activecoins[i]);
        LP_coinfind(activecoins[i]);
        LP_priceinfoadd(activecoins[i]);
        //test_validate("02000000010e62f95ff5881de8853ce1a5ddbaad731a62879d719367f539103600f1895477010000006b483045022100c684a0871689519bd97f2e61275752124f0f1498360750c87cf99a8acf06fd8c022047e7e62a7bfd481599130e6f40c95833f6ed6f44aa8b6ead7b0ec86a738b98a041210361857e1ba609aadff520a2ca9886fe7548c7154fab2cbe108c3b0e1e7635eb1ffeffffff02a0860100000000001976a9146cfa0a987f4c8f2ffee7e9944ef0c86fcda9671d88ac1e6f0700000000001976a9147f4b7113f9e26d84b150f2cc6d219baaf27f884488ace6b00700");
        //getchar();
        if ( (coin= LP_coinfind(activecoins[i])) != 0 )
        {
            if ( LP_getheight(coin) <= 0 )
                coin->inactive = (uint32_t)time(NULL);
            else LP_unspents_load(coin->symbol,coin->smartaddr);
            if ( coin->txfee == 0 && strcmp(coin->symbol,"BTC") != 0 )
                coin->txfee = LP_MIN_TXFEE;
        }
    }
    if ( (n= cJSON_GetArraySize(coins)) > 0 )
    {
        for (i=0; i<n; i++)
        {
            item = jitem(coins,i);
            if ( (symbol= jstr(item,"coin")) != 0 )
            {
                printf("%s ",jstr(item,"coin"));
                LP_coincreate(item);
                LP_priceinfoadd(jstr(item,"coin"));
                if ( (coin= LP_coinfind(symbol)) != 0 )
                {
                    if ( LP_getheight(coin) <= 0 )
                        coin->inactive = (uint32_t)time(NULL);
                    else LP_unspents_load(coin->symbol,coin->smartaddr);
                    if ( coin->txfee == 0 && strcmp(coin->symbol,"BTC") != 0 )
                        coin->txfee = LP_MIN_TXFEE;
                }
            }
        }
    }
    printf("privkey updates\n");
}

void LP_initpeers(int32_t pubsock,struct LP_peerinfo *mypeer,char *myipaddr,uint16_t myport,char *seednode,uint16_t pushport,uint16_t subport)
{
    int32_t i,j; uint32_t r;
    if ( IAMLP != 0 )
    {
        LP_mypeer = mypeer = LP_addpeer(mypeer,pubsock,myipaddr,myport,pushport,subport,1,G.LP_sessionid);
        if ( myipaddr == 0 || mypeer == 0 )
        {
            printf("couldnt get myipaddr or null mypeer.%p\n",mypeer);
            exit(-1);
        }
        if ( seednode == 0 || seednode[0] == 0 )
        {
            for (i=0; i<sizeof(default_LPnodes)/sizeof(*default_LPnodes); i++)
            {
                LP_addpeer(mypeer,pubsock,default_LPnodes[i],myport,pushport,subport,0,G.LP_sessionid);
            }
        } else LP_addpeer(mypeer,pubsock,seednode,myport,pushport,subport,0,G.LP_sessionid);
    }
    else
    {
        if ( myipaddr == 0 )
        {
            printf("couldnt get myipaddr\n");
            exit(-1);
        }
        if ( seednode == 0 || seednode[0] == 0 )
        {
            //LP_addpeer(mypeer,pubsock,"51.15.86.136",myport,pushport,subport,0,G.LP_sessionid);
            OS_randombytes((void *)&r,sizeof(r));
            //r = 0;
            for (j=0; j<sizeof(default_LPnodes)/sizeof(*default_LPnodes); j++)
            {
                i = (r + j) % (sizeof(default_LPnodes)/sizeof(*default_LPnodes));
                LP_addpeer(mypeer,pubsock,default_LPnodes[i],myport,pushport,subport,0,G.LP_sessionid);
                //issue_LP_getpeers(default_LPnodes[i],myport);
                //LP_peersquery(mypeer,pubsock,default_LPnodes[i],myport,"127.0.0.1",myport);
            }
        } else LP_addpeer(mypeer,pubsock,seednode,myport,pushport,subport,0,G.LP_sessionid);
    }
}

void LP_pubkeysloop(void *ctx)
{
    static uint32_t lasttime; cJSON *retjson; struct iguana_info *coin,*tmp;
    strcpy(LP_pubkeysloop_stats.name,"LP_pubkeysloop");
    LP_pubkeysloop_stats.threshold = 15000.;
    sleep(10);
    while ( 1 )
    {
        LP_millistats_update(&LP_pubkeysloop_stats);
        HASH_ITER(hh,LP_coins,coin,tmp) // firstrefht,firstscanht,lastscanht
        {
            if ( coin->electrum != 0 && time(NULL) > coin->lastunspent+30 )
            {
                //printf("call electrum listunspent.%s\n",coin->symbol);
                if ( (retjson= electrum_address_listunspent(coin->symbol,coin->electrum,&retjson,coin->smartaddr,2)) != 0 )
                    free_json(retjson);
                coin->lastunspent = (uint32_t)time(NULL);
            }
        }
        if ( time(NULL) > lasttime+LP_ORDERBOOK_DURATION*0.5 )
        {
//printf("LP_pubkeysloop %u\n",(uint32_t)time(NULL));
            LP_notify_pubkeys(ctx,LP_mypubsock);
            lasttime = (uint32_t)time(NULL);
        }
        sleep(3);
    }
}

void LP_privkeysloop(void *ctx)
{
    strcpy(LP_privkeysloop_stats.name,"LP_privkeysloop");
    LP_privkeysloop_stats.threshold = (LP_ORDERBOOK_DURATION * .8 * 1000) + 10000;
    sleep(20);
    while ( 1 )
    {
        LP_millistats_update(&LP_privkeysloop_stats);
        LP_counter += 1000;
//printf("LP_privkeysloop %u\n",LP_counter);
        LP_privkey_updates(ctx,LP_mypubsock,0);
        sleep(LP_ORDERBOOK_DURATION * .777);
    }
}

void LP_swapsloop(void *ignore)
{
    char *retstr;
    strcpy(LP_swapsloop_stats.name,"LP_swapsloop");
    LP_swapsloop_stats.threshold = 605000.;
    sleep(50);
    while ( 1 )
    {
        LP_millistats_update(&LP_swapsloop_stats);
        LP_counter += 10000;
//printf("LP_swapsloop %u\n",LP_counter);
        if ( (retstr= basilisk_swapentry(0,0)) != 0 )
            free(retstr);
        sleep(600);
    }
}

void LP_reserved_msgs(void *ignore)
{
    bits256 zero; int32_t flag,nonz; struct nn_pollfd pfd;
    memset(zero.bytes,0,sizeof(zero));
    strcpy(LP_reserved_msgs_stats.name,"LP_reserved_msgs");
    LP_reserved_msgs_stats.threshold = 150.;
    while ( 1 )
    {
        nonz = 0;
        LP_millistats_update(&LP_reserved_msgs_stats);
        if ( num_Reserved_msgs[0] > 0 || num_Reserved_msgs[1] > 0 )
        {
            nonz++;
            flag = 0;
            if ( LP_mypubsock >= 0 )
            {
                memset(&pfd,0,sizeof(pfd));
                pfd.fd = LP_mypubsock;
                pfd.events = NN_POLLOUT;
                if ( nn_poll(&pfd,1,1) == 1 )
                    flag = 1;
            } else flag = 1;
            if ( flag == 1 )
            {
                portable_mutex_lock(&LP_reservedmutex);
                if ( num_Reserved_msgs[1] > 0 )
                {
                    num_Reserved_msgs[1]--;
//printf("PRIORITY BROADCAST.(%s)\n",Reserved_msgs[1][num_Reserved_msgs[1]]);
                    LP_broadcast_message(LP_mypubsock,"","",zero,Reserved_msgs[1][num_Reserved_msgs[1]]);
                    Reserved_msgs[1][num_Reserved_msgs[1]] = 0;
                }
                else if ( num_Reserved_msgs[0] > 0 )
                {
                    num_Reserved_msgs[0]--;
//printf("BROADCAST.(%s)\n",Reserved_msgs[0][num_Reserved_msgs[0]]);
                    LP_broadcast_message(LP_mypubsock,"","",zero,Reserved_msgs[0][num_Reserved_msgs[0]]);
                    Reserved_msgs[0][num_Reserved_msgs[0]] = 0;
                }
                portable_mutex_unlock(&LP_reservedmutex);
            }
        }
        if ( ignore == 0 )
            break;
        if ( nonz != 0 )
            usleep(1000);
        else usleep(5000);
    }
}

int32_t LP_reserved_msg(int32_t priority,char *base,char *rel,bits256 pubkey,char *msg)
{
    int32_t n = 0;
    portable_mutex_lock(&LP_reservedmutex);
    if ( num_Reserved_msgs[priority] < sizeof(Reserved_msgs[priority])/sizeof(*Reserved_msgs[priority]) )
    {
        Reserved_msgs[priority][num_Reserved_msgs[priority]++] = msg;
        n = num_Reserved_msgs[priority];
    } else LP_broadcast_message(LP_mypubsock,base,rel,pubkey,msg);
    if ( num_Reserved_msgs[priority] > max_Reserved_msgs[priority] )
    {
        max_Reserved_msgs[priority] = num_Reserved_msgs[priority];
        if ( (max_Reserved_msgs[priority] % 100) == 0 )
            printf("New priority.%d max_Reserved_msgs.%d\n",priority,max_Reserved_msgs[priority]);
    }
    portable_mutex_unlock(&LP_reservedmutex);
    return(n);
}

void LPinit(uint16_t myport,uint16_t mypullport,uint16_t mypubport,uint16_t mybusport,char *passphrase,int32_t amclient,char *userhome,cJSON *argjson)
{
    char *myipaddr=0,version[64]; long filesize,n; int32_t valid,timeout,pubsock=-1; struct LP_peerinfo *mypeer=0; char pushaddr[128],subaddr[128],bindaddr[128],*coins_str=0; cJSON *coinsjson=0; void *ctx = bitcoin_ctx();
    sprintf(version,"Marketmaker %s.%s %s rsize.%ld",LP_MAJOR_VERSION,LP_MINOR_VERSION,LP_BUILD_NUMBER,sizeof(struct basilisk_request));
    printf("%s %u\n",version,calc_crc32(0,version,(int32_t)strlen(version)));
    if ( LP_MAXPRICEINFOS > 256 )
    {
        printf("LP_MAXPRICEINFOS %d wont fit in a uint8_t, need to increase the width of the baseind and relind for struct LP_pubkey_quote\n",LP_MAXPRICEINFOS);
        exit(-1);
    }
    LP_showwif = juint(argjson,"wif");
    if ( passphrase == 0 || passphrase[0] == 0 )
    {
        printf("jeezy says we cant use the nullstring as passphrase and I agree\n");
        exit(-1);
    }
    IAMLP = !amclient;
#ifndef __linux__
    if ( IAMLP != 0 )
    {
        printf("must run a unix node for LP node\n");
        exit(-1);
    }
#endif
    OS_randombytes((void *)&n,sizeof(n));
    srand((uint32_t)n);
    if ( jobj(argjson,"gui") != 0 )
        safecopy(LP_gui,jstr(argjson,"gui"),sizeof(LP_gui));
    if ( jobj(argjson,"canbind") == 0 )
    {
#ifndef __linux__
        LP_canbind = IAMLP;
#else
        LP_canbind = IAMLP;
#endif
    }
    else
    {
        LP_canbind = jint(argjson,"canbind");
        printf(">>>>>>>>>>> set LP_canbind.%d\n",LP_canbind);
    }
    if ( LP_canbind > 1000 && LP_canbind < 65536 )
        LP_fixed_pairport = LP_canbind;
    if ( LP_canbind != 0 )
        LP_canbind = 1;
    srand((int32_t)n);
    if ( userhome != 0 && userhome[0] != 0 )
    {
        safecopy(USERHOME,userhome,sizeof(USERHOME));
#ifdef __APPLE__
        strcat(USERHOME,"/Library/Application Support");
#endif
    }
    portable_mutex_init(&LP_peermutex);
    portable_mutex_init(&LP_utxomutex);
    portable_mutex_init(&LP_UTXOmutex);
    portable_mutex_init(&LP_commandmutex);
    portable_mutex_init(&LP_swaplistmutex);
    portable_mutex_init(&LP_cachemutex);
    portable_mutex_init(&LP_networkmutex);
    portable_mutex_init(&LP_gcmutex);
    portable_mutex_init(&LP_forwardmutex);
    portable_mutex_init(&LP_inusemutex);
    portable_mutex_init(&LP_psockmutex);
    portable_mutex_init(&LP_coinmutex);
    portable_mutex_init(&LP_pubkeymutex);
    portable_mutex_init(&LP_electrummutex);
    portable_mutex_init(&LP_messagemutex);
    portable_mutex_init(&LP_portfoliomutex);
    portable_mutex_init(&LP_butxomutex);
    portable_mutex_init(&LP_reservedmutex);
    portable_mutex_init(&LP_nanorecvsmutex);
    portable_mutex_init(&LP_tradebotsmutex);
    portable_mutex_init(&LP_cJSONmutex);
    myipaddr = clonestr("127.0.0.1");
#ifndef _WIN32
#ifndef FROM_JS
    if ( system("curl -s4 checkip.amazonaws.com > myipaddr") == 0 )
    {
        char ipfname[64];
        strcpy(ipfname,"myipaddr");
        if ( (myipaddr= OS_filestr(&filesize,ipfname)) != 0 && myipaddr[0] != 0 )
        {
            n = strlen(myipaddr);
            if ( myipaddr[n-1] == '\n' )
                myipaddr[--n] = 0;
            strcpy(LP_myipaddr,myipaddr);
        } else printf("error getting myipaddr\n");
    } else printf("error issuing curl\n");
#else
    IAMLP = 0;
#endif
#endif
    if ( IAMLP != 0 )
    {
        pubsock = -1;
        nanomsg_transportname(0,subaddr,myipaddr,mypubport);
        nanomsg_transportname(1,bindaddr,myipaddr,mypubport);
        //nanomsg_transportname2(1,bindaddr2,myipaddr,mypubport);
        valid = 0;
        if ( (pubsock= nn_socket(AF_SP,NN_PUB)) >= 0 )
        {
            valid = 0;
            if ( nn_bind(pubsock,bindaddr) >= 0 )
                valid++;
            //if ( nn_bind(pubsock,bindaddr2) >= 0 )
            //    valid++;
            if ( valid > 0 )
            {
                timeout = 1;
                nn_setsockopt(pubsock,NN_SOL_SOCKET,NN_SNDTIMEO,&timeout,sizeof(timeout));
            }
            else
            {
                printf("error binding to (%s).%d\n",subaddr,pubsock);
                if ( pubsock >= 0 )
                    nn_close(pubsock), pubsock = -1;
            }
        } else printf("error getting pubsock %d\n",pubsock);
        printf(">>>>>>>>> myipaddr.(%s) (%s) pullsock.%d valid.%d\n",bindaddr,subaddr,pubsock,valid);
        LP_mypubsock = pubsock;
    }
    printf("got %s, initpeers\n",myipaddr);
    LP_initpeers(pubsock,mypeer,myipaddr,myport,jstr(argjson,"seednode"),mypullport,mypubport);
    RPC_port = myport;
    printf("get public socket\n");
    LP_mypullsock = LP_initpublicaddr(ctx,&mypullport,pushaddr,myipaddr,mypullport,0);
    strcpy(LP_publicaddr,pushaddr);
    LP_publicport = mypullport;
    LP_mybussock = LP_coinbus(mybusport);
    //LP_deadman_switch = (uint32_t)time(NULL);
    printf("canbind.%d my command address is (%s) pullsock.%d pullport.%u\n",LP_canbind,pushaddr,LP_mypullsock,mypullport);
    if ( (coinsjson= jobj(argjson,"coins")) == 0 )
    {
        if ( (coins_str= OS_filestr(&filesize,"coins.json")) != 0 || (coins_str= OS_filestr(&filesize,"exchanges/coins.json")) != 0 )
        {
            unstringify(coins_str);
            printf("UNSTRINGIFIED.(%s)\n",coins_str);
            coinsjson = cJSON_Parse(coins_str);
            free(coins_str);
            // yes I know this coinsjson is not freed, not sure about if it is referenced
        }
    }
    if ( coinsjson == 0 )
    {
        printf("no coins object or coins.json file, must abort\n");
        exit(-1);
    }
    LP_initcoins(ctx,pubsock,coinsjson);

    G.waiting = 1;
    LP_passphrase_init(passphrase,jstr(argjson,"gui"));
#ifndef FROM_JS
    if ( IAMLP != 0 && OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)LP_psockloop,(void *)myipaddr) != 0 )
    {
        printf("error launching LP_psockloop for (%s)\n",myipaddr);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)LP_reserved_msgs,(void *)myipaddr) != 0 )
    {
        printf("error launching LP_reserved_msgs for (%s)\n",myipaddr);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)utxosQ_loop,(void *)myipaddr) != 0 )
    {
        printf("error launching utxosQ_loop for (%s)\n",myipaddr);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)stats_rpcloop,(void *)&myport) != 0 )
    {
        printf("error launching stats rpcloop for port.%u\n",myport);
        exit(-1);
    }
    uint16_t myport2 = myport-1;
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)stats_rpcloop,(void *)&myport2) != 0 )
    {
        printf("error launching stats rpcloop for port.%u\n",myport);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)command_rpcloop,(void *)myipaddr) != 0 )
    {
        printf("error launching command_rpcloop for port.%u\n",myport);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)queue_loop,(void *)myipaddr) != 0 )
    {
        printf("error launching queue_loop for port.%u\n",myport);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)gc_loop,(void *)myipaddr) != 0 )
    {
        printf("error launching gc_loop for port.%u\n",myport);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)prices_loop,ctx) != 0 )
    {
        printf("error launching prices_loop for port.%u\n",myport);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)LP_coinsloop,(void *)"") != 0 )
    {
        printf("error launching LP_coinsloop for port.%u\n",myport);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)LP_coinsloop,(void *)"BTC") != 0 )
    {
        printf("error launching BTC LP_coinsloop for port.%u\n",myport);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)LP_coinsloop,(void *)"KMD") != 0 )
    {
        printf("error launching KMD LP_coinsloop for port.%u\n",myport);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)LP_pubkeysloop,(void *)myipaddr) != 0 )
    {
        printf("error launching LP_pubkeysloop for ctx.%p\n",ctx);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)LP_privkeysloop,(void *)myipaddr) != 0 )
    {
        printf("error launching LP_privkeysloop for ctx.%p\n",ctx);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)LP_swapsloop,(void *)myipaddr) != 0 )
    {
        printf("error launching LP_swapsloop for port.%u\n",myport);
        exit(-1);
    }
    int32_t nonz;
    while ( 1 )
    {
        nonz = 0;
        G.waiting = 1;
        while ( G.initializing != 0 )
        {
            //fprintf(stderr,".");
            sleep(3);
        }
        if ( LP_mainloop_iter(ctx,myipaddr,mypeer,pubsock,pushaddr,myport) != 0 )
            nonz++;
        if ( nonz == 0 )
            usleep(1000);
        else if ( IAMLP == 0 )
            usleep(1000);
    }
#endif
}

#ifdef FROM_JS
extern void *Nanomsg_threadarg;
void *nn_thread_main_routine(void *arg);

void emscripten_usleep(int32_t x)
{
}

char *bitcoind_RPC(char **retstrp,char *debugstr,char *url,char *userpass,char *command,char *params,int32_t timeout)
{
    static uint32_t counter; char fname[512],*retstr; long fsize;
    if ( strncmp("http://",url,strlen("http://")) != 0 )
        return(clonestr("{\"error\":\"only http allowed\"}"));
    sprintf(fname,"bitcoind_RPC/request.%d",counter % 10);
    counter++;
    //printf("issue.(%s)\n",url);
    emscripten_wget(url,fname);
    retstr = OS_filestr(&fsize,fname);
    //printf("bitcoind_RPC(%s) -> fname.(%s) %s\n",url,fname,retstr);
    return(retstr);
}

char *barterDEX(char *argstr)
{
    static void *ctx;
    cJSON *argjson; char *retstr;
    if ( ctx == 0 )
        ctx = bitcoin_ctx();
    printf("barterDEX.(%s)\n",argstr);
    if ( (argjson= cJSON_Parse(argstr)) != 0 )
    {
        retstr = LP_command_process(ctx,LP_myipaddr,LP_mypubsock,argjson,(uint8_t *)argstr,(int32_t)strlen(argstr));
        free_json(argjson);
    } else retstr = clonestr("{\"error\":\"couldnt parse request\"}");
    return(retstr);
}

void LP_fromjs_iter()
{
    static void *ctx; char *retstr;
    if ( G.initializing != 0 )
    {
        printf("LP_fromjs_iter during G.initializing, skip\n");
        return;
    }
    if ( ctx == 0 )
        ctx = bitcoin_ctx();
    if ( 0 && (LP_counter % 100) == 0 )
        printf("LP_fromjs_iter got called LP_counter.%d userpass.(%s) ctx.%p\n",LP_counter,G.USERPASS,ctx);
    //if ( Nanomsg_threadarg != 0 )
    //    nn_thread_main_routine(Nanomsg_threadarg);
    //LP_pubkeys_query();
    //LP_utxosQ_process();
    //LP_nanomsg_recvs(ctx);
    LP_mainloop_iter(ctx,LP_myipaddr,0,LP_mypubsock,LP_publicaddr,LP_RPCPORT);
    //queue_loop(0);
    if ( 0 && (LP_counter % 10) == 0 ) // 10 seconds
    {
        LP_coinsloop(0);
        if ( (LP_counter % 100) == 0 ) // 100 seconds
        {
            LP_notify_pubkeys(ctx,LP_mypubsock);
            LP_privkey_updates(ctx,LP_mypubsock,0);
            if ( (retstr= basilisk_swapentry(0,0)) != 0 )
                free(retstr);
        }
    }
    LP_counter++;
}

#endif

#undef calloc
#undef free
#undef realloc
#undef clonestr

struct LP_memory_list
{
    struct LP_memory_list *next,*prev;
    uint32_t timestamp,len;
    void *ptr;
} *LP_memory_list;
int32_t zeroval() { return(0); }

void *LP_alloc(uint64_t len)
{
//return(calloc(1,len));
    LP_cjson_allocated += len;
    LP_cjson_total += len;
    LP_cjson_count++;
    struct LP_memory_list *mp;
    mp = calloc(1,sizeof(*mp) + len);
    mp->ptr = calloc(1,len);
    //printf(">>>>>>>>>>> LP_alloc mp.%p ptr.%p len.%llu %llu\n",mp,mp->ptr,(long long)len,(long long)LP_cjson_allocated);
    mp->timestamp = (uint32_t)time(NULL);
    mp->len = (uint32_t)len;
    portable_mutex_lock(&LP_cJSONmutex);
    DL_APPEND(LP_memory_list,mp);
    portable_mutex_unlock(&LP_cJSONmutex);
    return(mp->ptr);
}

void LP_free(void *ptr)
{
    static uint32_t lasttime,unknown; static int64_t lasttotal;
//free(ptr); return;
    uint32_t now; char str[65]; int32_t n,lagging; uint64_t total = 0; struct LP_memory_list *mp,*tmp;
    if ( (now= (uint32_t)time(NULL)) > lasttime+1 )
    {
        n = lagging = 0;
        DL_FOREACH_SAFE(LP_memory_list,mp,tmp)
        {
            total += mp->len;
            n++;
            if ( 0 && now > mp->timestamp+120 )
            {
                lagging++;
                if ( now > mp->timestamp+240 )
                {
                    portable_mutex_lock(&LP_cJSONmutex);
                    DL_DELETE(LP_memory_list,mp);
                    portable_mutex_unlock(&LP_cJSONmutex);
                    free(mp->ptr);
                    free(mp);
                }
            }
        }
        printf("[%lld] total %d allocated total %llu/%llu [%llu %llu] %.1f ave %s unknown.%u lagging.%d\n",(long long)(total-lasttotal),n,(long long)total,(long long)LP_cjson_allocated,(long long)LP_cjson_total,(long long)LP_cjson_count,(double)LP_cjson_total/LP_cjson_count,mbstr(str,total),unknown,lagging);
        lasttime = (uint32_t)time(NULL);
        lasttotal = total;
    }
    DL_FOREACH_SAFE(LP_memory_list,mp,tmp)
    {
        if ( mp->ptr == ptr )
            break;
        mp = 0;
    }
    if ( mp != 0 )
    {
        LP_cjson_allocated -= mp->len;
        portable_mutex_lock(&LP_cJSONmutex);
        DL_DELETE(LP_memory_list,mp);
        portable_mutex_unlock(&LP_cJSONmutex);
        //printf(">>>>>>>>>>> LP_free ptr.%p mp.%p len.%u %llu\n",ptr,mp,mp->len,(long long)LP_cjson_allocated);
        free(mp->ptr);
        free(mp);
    } else unknown++; // free from source file with #define redirect for alloc that wasnt
}

/*char *LP_clonestr(char *str)
{
    char *retstr = LP_alloc(strlen(str)+1);
    strcpy(retstr,str);
    return(retstr);
}

void *LP_realloc(void *ptr,uint64_t len)
{
    return(realloc(ptr,len));
}*/


