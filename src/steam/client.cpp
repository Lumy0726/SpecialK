﻿#include <SpecialK/stdafx.h>

#include <SpecialK/steam_api.h>

class ISteamUser;
class IWrapSteamUser;

class ISteamUtils;
class IWrapSteamUtils;

class ISteamController;
class IWrapSteamController;

class ISteamRemoteStorage;
class IWrapSteamRemoteStorage;

class ISteamClient;
class IWrapSteamClient;

extern concurrency::concurrent_unordered_map <ISteamUser*,          IWrapSteamUser*>          SK_SteamWrapper_remap_user;
extern concurrency::concurrent_unordered_map <ISteamUtils*,         IWrapSteamUtils*>         SK_SteamWrapper_remap_utils;
extern concurrency::concurrent_unordered_map <ISteamController*,    IWrapSteamController*>    SK_SteamWrapper_remap_controller;
extern concurrency::concurrent_unordered_map <ISteamRemoteStorage*, IWrapSteamRemoteStorage*> SK_SteamWrapper_remap_remotestorage;

concurrency::concurrent_unordered_map <ISteamClient*, IWrapSteamClient*>&
_SK_Singleton_SteamWrapper_remap_client (void)
{
  static concurrency::concurrent_unordered_map <ISteamClient*, IWrapSteamClient*>
         _remap_client;
  return _remap_client;
}

#define SK_SteamWrapper_remap_client _SK_Singleton_SteamWrapper_remap_client()


extern
ISteamUser*
SK_SteamWrapper_WrappedClient_GetISteamUser ( ISteamClient *This,
                                              HSteamUser    hSteamUser,
                                              HSteamPipe    hSteamPipe,
                                              const char   *pchVersion );

extern
ISteamController*
SK_SteamWrapper_WrappedClient_GetISteamController ( ISteamClient *This,
                                                    HSteamUser    hSteamUser,
                                                    HSteamPipe    hSteamPipe,
                                                    const char   *pchVersion );

extern
ISteamUtils*
SK_SteamWrapper_WrappedClient_GetISteamUtils ( ISteamClient *This,
                                               HSteamPipe    hSteamPipe,
                                               const char   *pchVersion );

extern
ISteamRemoteStorage*
SK_SteamWrapper_WrappedClient_GetISteamRemoteStorage ( ISteamClient *This,
                                                       HSteamUser    hSteamuser,
                                                       HSteamPipe    hSteamPipe,
                                                       const char   *pchVersion );

extern
ISteamUGC*
SK_SteamWrapper_WrappedClient_GetISteamUGC ( ISteamClient *This,
                                             HSteamUser    hSteamuser,
                                             HSteamPipe    hSteamPipe,
                                             const char   *pchVersion );

//#define WRAP_CONTROLLER
//#define WRAP_STORAGE
#define WRAP_USER
#define WRAP_UTILS
//#define WRAP_UGC

class IWrapSteamClient : public ISteamClient
{
public:
  explicit IWrapSteamClient (ISteamClient* pSteamClient) :
                              pRealClient (pSteamClient) {
  };

  virtual ~IWrapSteamClient (void) = default;

  HSteamPipe CreateSteamPipe     (void)                                               override { return pRealClient->CreateSteamPipe     (          );                } // 0
  bool       BReleaseSteamPipe   (HSteamPipe   hSteamPipe)                            override { return pRealClient->BReleaseSteamPipe   (hSteamPipe);                } // 1
  HSteamUser ConnectToGlobalUser (HSteamPipe   hSteamPipe)                            override { return pRealClient->ConnectToGlobalUser (hSteamPipe);                } // 2
  HSteamUser CreateLocalUser     (HSteamPipe *phSteamPipe, EAccountType eAccountType) override { return pRealClient->CreateLocalUser     (phSteamPipe, eAccountType); } // 3
  void       ReleaseUser         (HSteamPipe   hSteamPipe, HSteamUser hUser)          override { return pRealClient->ReleaseUser         (hSteamPipe,  hUser);        } // 4



  ISteamUser *GetISteamUser      (       HSteamUser  hSteamUser,
                                         HSteamPipe  hSteamPipe,
                                   const char       *pchVersion ) override
  {
#ifdef WRAP_USER
    return SK_SteamWrapper_WrappedClient_GetISteamUser ( pRealClient,
                                                           hSteamUser,
                                                             hSteamPipe,
                                                               pchVersion );
#else
    return pRealClient->GetISteamUser ( hSteamUser,
                                          hSteamPipe,
                                            pchVersion );
#endif
  } // 5



  ISteamGameServer         *GetISteamGameServer          (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamGameServer         (hSteamUser, hSteamPipe, pchVersion);                                                } // 6
  void                      SetLocalIPBinding            (uint32 unIP, uint16 usPort)                                           override {
    return pRealClient->SetLocalIPBinding           (unIP, usPort);                                                                      } // 7
  ISteamFriends            *GetISteamFriends             (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamFriends            (hSteamUser, hSteamPipe, pchVersion);                                                } // 8
  ISteamUtils              *GetISteamUtils               (HSteamPipe hSteamPipe, const char *pchVersion)                        override
  {
#ifdef WRAP_UTILS
    return SK_SteamWrapper_WrappedClient_GetISteamUtils ( pRealClient,
                                                            hSteamPipe,
                                                              pchVersion );
#else
    return pRealClient->GetISteamUtils ( hSteamPipe,
                                           pchVersion );
#endif
  } // 9
  ISteamMatchmaking        *GetISteamMatchmaking         (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamMatchmaking        (hSteamUser, hSteamPipe, pchVersion);                                                } // 10
  ISteamMatchmakingServers *GetISteamMatchmakingServers  (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamMatchmakingServers (hSteamUser, hSteamPipe, pchVersion);                                                } // 11
  void                     *GetISteamGenericInterface    (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamGenericInterface   (hSteamUser, hSteamPipe, pchVersion);                                                } // 12
  ISteamUserStats          *GetISteamUserStats           (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamUserStats          (hSteamUser, hSteamPipe, pchVersion);                                                } // 13
  ISteamGameServerStats    *GetISteamGameServerStats     (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamGameServerStats    (hSteamuser, hSteamPipe, pchVersion);                                                } // 14
  ISteamApps               *GetISteamApps                (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamApps               (hSteamUser, hSteamPipe, pchVersion);                                                } // 15
  ISteamNetworking         *GetISteamNetworking          (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamNetworking         (hSteamUser, hSteamPipe, pchVersion);                                                } // 16
  ISteamRemoteStorage      *GetISteamRemoteStorage       (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
#ifdef WRAP_STORAGE
    return SK_SteamWrapper_WrappedClient_GetISteamRemoteStorage ( pRealClient,
                                                                    hSteamuser,
                                                                      hSteamPipe,
                                                                        pchVersion );
#else
    return pRealClient->GetISteamRemoteStorage ( hSteamuser,
                                                   hSteamPipe,
                                                     pchVersion );
#endif
  } // 17
  ISteamScreenshots        *GetISteamScreenshots         (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamScreenshots        (hSteamuser, hSteamPipe, pchVersion);                                                } // 18
  void                      RunFrame                     (void)                                                                 override {
    return pRealClient->RunFrame                    ();                                                                                  } // 19
  uint32                    GetIPCCallCount              (void)                                                                 override {
    return pRealClient->GetIPCCallCount             ();                                                                                  } // 20
  void                      SetWarningMessageHook        (SteamAPIWarningMessageHook_t pFunction)                               override {
    return pRealClient->SetWarningMessageHook       (pFunction);                                                                         } // 21
  bool                      BShutdownIfAllPipesClosed    (void)                                                override { return false;  } // 22
  ISteamHTTP               *GetISteamHTTP                (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamHTTP               (hSteamuser, hSteamPipe, pchVersion);                                                } // 23
  ISteamUnifiedMessages    *GetISteamUnifiedMessages     (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamUnifiedMessages    (hSteamuser, hSteamPipe, pchVersion);                                                } // 24




  ISteamController         *GetISteamController          (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
#ifdef WRAP_CONTROLLER
    return SK_SteamWrapper_WrappedClient_GetISteamController (pRealClient, hSteamUser, hSteamPipe, pchVersion);
#else
    return pRealClient->GetISteamController (hSteamUser, hSteamPipe, pchVersion);
#endif
                                                                                                                                         } // 25






  ISteamUGC                *GetISteamUGC                 (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
#ifdef WRAP_UGC
    return SK_SteamWrapper_WrappedClient_GetISteamUGC ( pRealClient,
                                                          hSteamUser,
                                                            hSteamPipe,
                                                              pchVersion );
#else
    return pRealClient->GetISteamUGC                (hSteamUser, hSteamPipe, pchVersion);
#endif
  } // 26
  ISteamAppList            *GetISteamAppList             (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamAppList            (hSteamUser, hSteamPipe, pchVersion);                                                } // 27
  ISteamMusic              *GetISteamMusic               (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamMusic              (hSteamuser, hSteamPipe, pchVersion);                                                } // 28
  ISteamMusicRemote        *GetISteamMusicRemote         (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamMusicRemote        (hSteamuser, hSteamPipe, pchVersion);                                                } // 29
  ISteamHTMLSurface        *GetISteamHTMLSurface         (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamHTMLSurface        (hSteamuser, hSteamPipe, pchVersion);                                                } // 30

  void                      Set_SteamAPI_CPostAPIResultInProcess           (SteamAPI_PostAPIResultInProcess_t  func) override {
    return pRealClient->Set_SteamAPI_CPostAPIResultInProcess           (func);                                                } // 31
  void                      Remove_SteamAPI_CPostAPIResultInProcess        (SteamAPI_PostAPIResultInProcess_t  func) override {
    return pRealClient->Remove_SteamAPI_CPostAPIResultInProcess        (func);                                                } // 32
  void                      Set_SteamAPI_CCheckCallbackRegisteredInProcess (SteamAPI_CheckCallbackRegistered_t func) override {
    return pRealClient->Set_SteamAPI_CCheckCallbackRegisteredInProcess (func);                                                } // 33

  ISteamInventory          *GetISteamInventory           (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamInventory          (hSteamuser, hSteamPipe, pchVersion);                                                } // 34
  ISteamVideo              *GetISteamVideo               (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamVideo              (hSteamuser, hSteamPipe, pchVersion);                                                } // 35
	ISteamParentalSettings   *GetISteamParentalSettings    (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamParentalSettings   (hSteamuser, hSteamPipe, pchVersion);                                                } // 36


  ISteamClient* pRealClient;
};



using SteamClient_pfn = ISteamClient* (S_CALLTYPE *)(
        void
      );
SteamClient_pfn SteamClient_Original = nullptr;

ISteamClient*
S_CALLTYPE
SteamClient_Detour (void)
{
  SK_RunOnce (
    steam_log->Log ( L"[!] %hs ()",
                       __FUNCTION__ )
  );

  ISteamClient* pClient =
    static_cast <ISteamClient *> ( SteamClient_Original () );

  if (pClient != nullptr)
  {
    if (SK_SteamWrapper_remap_client.count (pClient))
       return SK_SteamWrapper_remap_client [pClient];

    else
    {
      SK_SteamWrapper_remap_client [pClient] =
              new IWrapSteamClient (pClient);

      return SK_SteamWrapper_remap_client [pClient];
    }
  }

  return nullptr;
}


using SteamInternal_CreateInterface_pfn = void*       (S_CALLTYPE *)(
  const char *ver
  );
SteamInternal_CreateInterface_pfn         SteamInternal_CreateInterface_Original = nullptr;


void*
S_CALLTYPE
SteamInternal_CreateInterface_Detour (const char *ver)
{
  steam_log->Log (L"[Steam Wrap] [!] SteamInternal_CreateInterface (%hs)", ver);

  //return
  //  SteamInternal_CreateInterface_Original (ver);





  if (! lstrcmpA (ver, STEAMCLIENT_INTERFACE_VERSION))
  {
    ISteamClient* pClient =
      static_cast <ISteamClient *> ( SteamInternal_CreateInterface_Original (ver) );

    if (pClient != nullptr)
    {
      if (SK_SteamWrapper_remap_client.count (pClient))
         return SK_SteamWrapper_remap_client [pClient];

      else
      {
        SK_SteamWrapper_remap_client [pClient] =
                new IWrapSteamClient (pClient);

        return SK_SteamWrapper_remap_client [pClient];
      }
    }
  }

  else
  {
    SK_RunOnce (
      steam_log->Log ( L"Game requested unexpected interface version (%s)!",
                         ver )
    );
  }

  return
    SteamInternal_CreateInterface_Original (ver);
}
