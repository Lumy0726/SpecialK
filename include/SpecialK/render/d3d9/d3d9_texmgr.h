﻿#pragma once

#include <Windows.h>

#include <SpecialK/log.h>
#include <SpecialK/config.h>
#include <SpecialK/framerate.h>
#include <SpecialK/tls.h>
#include <SpecialK/utility.h>
#include <SpecialK/thread.h>

//#include "render.h"

#include <d3d9.h>

#include <SpecialK/render/d3d9/d3d9_backend.h>

#include <set>
#include <map>
#include <queue>
#include <limits>
#include <cstdint>
#include <algorithm>
#include <unordered_map>

#include <concurrent_unordered_set.h>
#include <concurrent_unordered_map.h>
#include <concurrent_queue.h>

using namespace concurrency;

interface ISKTextureD3D9;

template <typename _T>
class SK_ThreadSafe_HashSet
{
public:
  SK_ThreadSafe_HashSet (void) {
    lock_ =
      std::make_unique <SK_Thread_HybridSpinlock> (0x100);
  }

  ~SK_ThreadSafe_HashSet (void) = default;

  void emplace (_T item)
  {
    std::scoped_lock <SK_Thread_HybridSpinlock>
          scope_lock (*lock_);

    container_.emplace (item);
  }

  void erase (_T item)
  {
    std::scoped_lock <SK_Thread_HybridSpinlock>
          scope_lock (*lock_);

    container_.erase (item);
  }

  bool contains (_T item)
  {
    std::scoped_lock <SK_Thread_HybridSpinlock>
          scope_lock (*lock_);

    return
      container_.count (item) != 0;
  }

  bool empty (void)
  {
    std::scoped_lock <SK_Thread_HybridSpinlock>
          scope_lock (*lock_);

    return
      container_.empty ();
  }

protected:
private:
  std::unordered_set <_T>                       container_;
  std::unique_ptr    <SK_Thread_HybridSpinlock> lock_;
};

typedef struct _D3DXIMAGE_INFO       D3DXIMAGE_INFO;
typedef enum   _D3DXIMAGE_FILEFORMAT D3DXIMAGE_FILEFORMAT;

namespace SK   {
namespace D3D9 {

class Texture {
public:
  Texture (void)
  {
    crc32c        = 0;
    size          = 0;
    refs          = 0;
    load_time     = 0.0f;
    d3d9_tex      = nullptr;
    original_pool = D3DPOOL_DEFAULT;
  }

  ISKTextureD3D9* d3d9_tex;
  size_t          size;
  std::atomic_int refs;
  float           load_time;
  uint32_t        crc32c;
  D3DPOOL         original_pool;
};

struct TexThreadStats {
  ULONGLONG bytes_loaded;
  LONG      jobs_retired;

  struct {
    FILETIME start, end;
    FILETIME user,  kernel;
    FILETIME idle; // Computed: (now - start) - (user + kernel)
  } runtime;
};

#if 0
  class Sampler {
    int                id;
    IDirect3DTexture9* pTex;
  };
#endif

  extern SK_LazyGlobal <std::set <UINT>> active_samplers;


  enum TexLoadMethod {
    Streaming,
    Blocking,
    DontCare
  };

  struct TexRecord {
             size_t        size    = 1;
    unsigned int           archive = std::numeric_limits <unsigned int>::max ();
             int           fileno  = 0UL;
    enum     TexLoadMethod method  = DontCare;
    volatile LONG          removed = FALSE; // Rather than removing entries and
                                           //   shuffling lists in memory, alter this
  };

  using TexList = std::vector < std::pair < uint32_t, TexRecord > >;


  struct TexLoadRequest
  {
    enum {
      Stream,    // This load will be streamed
      Immediate, // This load must finish immediately   (pSrc is unused)
      Resample   // Change image properties             (pData is supplied)
    } type = Stream;

    LPDIRECT3DDEVICE9   pDevice     = nullptr;

    // Resample only
    uint8_t            *pSrcData    = nullptr;
    UINT                SrcDataSize = 0UL;

    uint32_t            checksum    = 0UL;
    uint32_t            size        = 0UL;

    // Stream / Immediate
    wchar_t             wszFilename [MAX_PATH + 2] = { };

    D3DPOOL             mem_pool    = D3DPOOL_DEFAULT;
    LPDIRECT3DTEXTURE9  pDest       = nullptr;
    LPDIRECT3DTEXTURE9  pSrc        = nullptr;

    LARGE_INTEGER       start       = { 0LL };
    LARGE_INTEGER       end         = { 0LL };
  };

  class TexLoadRef
  {
  public:
     TexLoadRef (TexLoadRequest* ref)  { ref_ = ref;}
    ~TexLoadRef (void) = default;

    operator TexLoadRequest* (void)  {
      return ref_;
    }

  protected:
    TexLoadRequest* ref_ = nullptr;
  };


  class TextureManager
  {
  public:
    void Init     (void);
    void Hook     (void);
    void Shutdown (void);

    void                       removeTexture       (ISKTextureD3D9* pTexD3D9);

    Texture*                   getTexture          (uint32_t crc32c);
    void                       addTexture          (uint32_t crc32c, Texture* pTex, size_t size);

    bool                       reloadTexture       (uint32_t crc32c);

    // Record a cached reference
    void                       refTexture          (Texture* pTex);
    void                       refTextureEx        (Texture* pTex, bool add_to_ref_count = false);

    // Similar, just call this to indicate a cache miss
    void                       missTexture         (void)  {
      InterlockedIncrement (&misses);
    }

    void                       reset               (void);
    void                       purge               (void); // WIP

    size_t                     numTextures         (void) const {
      return textures.size ();
    }
    int                        numInjectedTextures (void) const;

    int64_t                    cacheSizeTotal      (void) const;
    int64_t                    cacheSizeBasic      (void) const;
    int64_t                    cacheSizeInjected   (void) const;

    int                        numMSAASurfs        (void);

    void                       addInjected         ( ISKTextureD3D9*    pWrapperTex,
                                                     IDirect3DTexture9* pOverrideTex,
                                                     size_t             size,
                                                     float              load_time );

    std::string                osdStats             (void) { return osd_stats; }
    void                       updateOSD            (void);


    float                      getTimeSaved         (void) const  { return time_saved;                   }
    LONG64                     getByteSaved         (void)                { return ReadAcquire64 (&bytes_saved); }
     LONG                      getHitCount          (void)                { return ReadAcquire   (&hits);        }
     LONG                      getMissCount         (void)                { return ReadAcquire   (&misses);      }


    void                       resetUsedTextures    (void);
    void                       applyTexture         (IDirect3DBaseTexture9* tex);

    size_t                     getUsedRenderTargets (std::vector <IDirect3DBaseTexture9 *>& targets) const;

    uint32_t                   getRenderTargetCreationTime
                                                    (IDirect3DBaseTexture9* rt);
    void                       trackRenderTarget    (IDirect3DBaseTexture9* rt);
    bool                       isRenderTarget       (IDirect3DBaseTexture9* rt) const;
    bool                       isUsedRenderTarget   (IDirect3DBaseTexture9* rt) const;

    void                       logUsedTextures      (void);

    void                       queueScreenshot      (wchar_t* wszFileName, bool hudless = true);
    bool                       wantsScreenshot      (void);
    HRESULT                    takeScreenshot       (IDirect3DSurface9* pSurf);

    void                       getThreadStats       (std::vector <TexThreadStats>& stats);


    HRESULT                    dumpTexture          (D3DFORMAT fmt, uint32_t checksum, IDirect3DTexture9* pTex);
    bool                       deleteDumpedTexture  (D3DFORMAT fmt, uint32_t checksum);
    bool                       isTextureDumped      (uint32_t  checksum);

    size_t                     getTextureArchives   (std::vector <std::wstring>& arcs);
    void                       refreshDataSources   (void);

    HRESULT                    injectTexture           (TexLoadRequest* load);
    bool                       isTextureInjectable     (uint32_t        checksum)    const;
    bool                       removeInjectableTexture (uint32_t        checksum);
    size_t                     getInjectableTextures   (TexList&        texure_list) const;
    TexRecord&                 getInjectableTexture    (uint32_t        checksum);

    bool                       isTextureBlacklisted    (uint32_t        checksum)    const;



    void                       updateQueueOSD       (void);
    int                        loadQueuedTextures   (void);


    BOOL                       isTexturePowerOfTwo (UINT sampler) const
    {
      return sampler_flags [sampler < 255 ? sampler : 255].power_of_two;
    }

    // Sttate of the active texture for each sampler,
    //   needed to correct some texture coordinate address
    //     problems in the base game.
    struct
    {
      BOOL power_of_two = FALSE;
    } sampler_flags [256] = { };


    // The set of textures used during the last frame
    std::vector             <uint32_t>                         textures_last_frame;
    //SK_ThreadSafe_HashSet <uint32_t>                         textures_used;
    concurrent_unordered_set <uint32_t>                        textures_used;
//  std::unordered_set <uint32_t>                              non_power_of_two_textures;

    // Textures that we will not allow injection for
    //   (primarily to speed things up, but also for EULA-related reasons).
    concurrent_unordered_set <uint32_t>                        inject_blacklist;


    //
    // Streaming System Internals
    //
    class Injector
    {
      friend class TextureManager;

    public:
      void            init               (void);

      bool            hasPendingLoads    (void)              const;

      bool            beginLoad          (void); // Returns original status
      void            endLoad            (bool bOrigState = false);

      bool            hasPendingStreams  (void)              const;
      bool            isStreaming        (uint32_t checksum) const;

      TexLoadRequest* getTextureInFlight (uint32_t        checksum);
      void            addTextureInFlight (TexLoadRequest* load_op);
      void            finishedStreaming  (uint32_t        checksum);

      inline bool isInjectionThread (void) const
      {
        return SK_TLS_Bottom ()->texture_management.injection_thread;
      }

      void lockStreaming    (void) { EnterCriticalSection (&cs_tex_stream);    };
      void lockResampling   (void) { EnterCriticalSection (&cs_tex_resample);  };
      void lockDumping      (void) { EnterCriticalSection (&cs_tex_dump);      };
      void lockBlacklist    (void) { EnterCriticalSection (&cs_tex_blacklist); };

      void unlockStreaming  (void) { LeaveCriticalSection (&cs_tex_stream);    };
      void unlockResampling (void) { LeaveCriticalSection (&cs_tex_resample);  };
      void unlockDumping    (void) { LeaveCriticalSection (&cs_tex_dump);      };
      void unlockBlacklist  (void) { LeaveCriticalSection (&cs_tex_blacklist); };

    //protected:
      concurrent_unordered_map   <uint32_t, TexLoadRequest *>
                                                        textures_in_flight;
      concurrent_queue <TexLoadRef>                     textures_to_stream;
      concurrent_queue <TexLoadRef>                     finished_loads;

      CRITICAL_SECTION                                  cs_tex_stream    = { };
      CRITICAL_SECTION                                  cs_tex_resample  = { };
      CRITICAL_SECTION                                  cs_tex_dump      = { };
      CRITICAL_SECTION                                  cs_tex_blacklist = { };

      volatile  LONG                                    streaming        = 0L;
      volatile ULONG                                    streaming_bytes  = 0UL;

      volatile  LONG                                    resampling       = 0L;
    } injector;

  private:
    struct {
      // In lieu of actually wrapping render targets with a COM interface, just add the creation time
      //   as the e9ped parameter
      concurrent_unordered_map <IDirect3DBaseTexture9 *, uint32_t> render_targets;
    } known;

    struct {
      concurrent_unordered_set <IDirect3DBaseTexture9 *>     render_targets;
    } used;

    concurrent_unordered_map <uint32_t, Texture*>            textures;

    float                                                    time_saved      = 0.0f;
    volatile LONG64                                          bytes_saved     = 0LL;

    volatile LONG                                            hits            = 0L;
    volatile LONG                                            misses          = 0L;

    volatile LONG64                                          basic_size      = 0LL;
    volatile LONG64                                          injected_size   = 0LL;
    volatile LONG                                            injected_count  = 0L;

    std::string                                              osd_stats       = "";
    bool                                                     want_screenshot = false;

public:
    CRITICAL_SECTION                                         cs_cache        = { };
    CRITICAL_SECTION                                         cs_free_list    = { };
    CRITICAL_SECTION                                         cs_unreferenced = { };

    concurrent_unordered_map <uint32_t, TexRecord>           injectable_textures;
    std::vector              <std::wstring>                  archives;
    concurrent_unordered_map <int32_t, bool>                 dumped_textures;

  public:
    bool                                                     init            = false;
    DWORD                                                    thread_id       = 0x0;
    ULONG64                                                  last_frame      = 0ULL;
  };


  class TextureWorkerThread
  {
  friend class TextureThreadPool;

  public:
     TextureWorkerThread (TextureThreadPool* pool);
    ~TextureWorkerThread (void);

    void startJob  (TexLoadRequest* job) {
      job_ = job;
      SetEvent (control_.start);
    }

    void trim (void) {
      SetEvent (control_.trim);
    }

    void finishJob (void);

    bool isBusy   (void) noexcept {
      return (job_ != nullptr);
    }

    void shutdown (void) {
      SetEvent (control_.shutdown);
    }

    size_t bytesLoaded (void) const noexcept {
      return sk::narrow_cast <size_t> (ReadAcquire64 (&bytes_loaded_));
    }

    int    jobsRetired  (void) const noexcept {
      return ReadAcquire (&jobs_retired_);
    }

    FILETIME idleTime   (void)
    {
      GetThreadTimes ( thread_,
                         &runtime_.start, &runtime_.end,
                           &runtime_.kernel, &runtime_.user );

      FILETIME now;
      GetSystemTimeAsFileTime (&now);

      //ULONGLONG elapsed =
      //  ULARGE_INTEGER { now.dwLowDateTime,            now.dwHighDateTime            }.QuadPart -
      //  ULARGE_INTEGER { runtime_.start.dwLowDateTime, runtime_.start.dwHighDateTime }.QuadPart;
      //
      //ULONGLONG busy =
      //  ULARGE_INTEGER { runtime_.kernel.dwLowDateTime, runtime_.kernel.dwHighDateTime }.QuadPart +
      //  ULARGE_INTEGER { runtime_.user.dwLowDateTime,   runtime_.user.dwHighDateTime   }.QuadPart;

      volatile LONG64 idle = 0LL;

      //InterlockedAdd64 (&idle, elapsed);
      //InterlockedAdd64 (&idle, -busy);

      return FILETIME { (DWORD) idle        & 0xFFFFFFFF,
                        (DWORD)(idle >> 31) & 0xFFFFFFFF };
    }
    [[nodiscard]] FILETIME userTime   (void) const noexcept { return runtime_.user;   };
    [[nodiscard]] FILETIME kernelTime (void) const noexcept { return runtime_.kernel; };

  protected:
    static volatile LONG  num_threads_init;

    static DWORD __stdcall ThreadProc (LPVOID user);

    TextureThreadPool*    pool_;

    DWORD                 thread_id_;
    HANDLE                thread_;

    TexLoadRequest*       job_;

    volatile LONGLONG     bytes_loaded_ = 0LL;
    volatile LONG         jobs_retired_ = 0L;

    std::wstring          name_;

    struct {
      FILETIME start, end;
      FILETIME user,  kernel;
      FILETIME idle; // Computed: (now - start) - (user + kernel)
    } runtime_ { 0, 0, 0, 0, 0 };

    struct {
      union {
        struct {
          HANDLE start;
          HANDLE trim;
          HANDLE shutdown;
        };
        HANDLE   ops [3];
      };
    } control_;
  };

  class TextureThreadPool
  {
  friend class TextureWorkerThread;

  public:
    TextureThreadPool (void)///
    {
      events_.jobs_added =
        SK_CreateEvent (nullptr, TRUE, FALSE, nullptr);

      events_.results_waiting =
        SK_CreateEvent (nullptr, TRUE, FALSE, nullptr);

      events_.shutdown =
        SK_CreateEvent (nullptr, FALSE, FALSE, nullptr);

      constexpr int MAX_THREADS = 5;///*config.textures.worker_threads*/ 4 / 2;

      if (! init_worker_sync_)
      {
        // We will add a sync. barrier that waits for all of the threads in this pool, plus all of the threads
        //   in the other pool to initialize. This design is flawed, but safe.
        init_worker_sync_ = true;
      }

      for (int i = 0; i < MAX_THREADS; i++)
      {
        workers_.emplace_back (
          std::make_unique <TextureWorkerThread> (this)
        );
      }

      // This will be deferred until it is first needed...
      spool_thread_ = nullptr;
    }

    ~TextureThreadPool (void)///
    {
      if (spool_thread_ != nullptr)
      {
        shutdown ();

        SK_WaitForSingleObject (spool_thread_, INFINITE);
        SK_CloseHandle         (spool_thread_);
      }

      SK_CloseHandle (events_.results_waiting);
      SK_CloseHandle (events_.jobs_added);
      SK_CloseHandle (events_.shutdown);
    }

    void postJob (TexLoadRequest* job);

    void getFinished (std::vector <TexLoadRequest *>& results)
    {
      DWORD dwResults = 0;
            dwResults =
        SK_WaitForSingleObject (events_.results_waiting, 0L);

      // Nothing waiting
      if (dwResults != WAIT_OBJECT_0 && (0 == ReadAcquire (&jobs_done_)))
        return;

      std::scoped_lock <SK_Thread_HybridSpinlock>
                 _lock (cs_results_);

      while (! results_.empty ())
      {
        SK::D3D9::TexLoadRef ref (nullptr);

        if (results_.try_pop (ref))
        {
          results.emplace_back (ref);
          InterlockedDecrement (&jobs_done_);
        }
      }

      ResetEvent (events_.results_waiting);
    }

    bool   working     (void) const noexcept { return ReadAcquire (&jobs_done_) > 0; }
    void   shutdown    (void) const noexcept { SetEvent (events_.shutdown);  }

    size_t queueLength (void) const noexcept
    {
      return
        ReadAcquire (&jobs_waiting_);
    }


    std::vector <SK::D3D9::TexThreadStats>
    getWorkerStats (void)
    {
      std::vector <SK::D3D9::TexThreadStats> stats;

      for ( auto& it : workers_ )
      {
        SK::D3D9::TexThreadStats stat = { };

        stat.bytes_loaded   = it->bytesLoaded ();
        stat.jobs_retired   = it->jobsRetired ();
        stat.runtime.idle   = it->idleTime    ();
        stat.runtime.kernel = it->kernelTime  ();
        stat.runtime.user   = it->userTime    ();

        stats.emplace_back (stat);
      }

      return stats;
    }


  protected:
    static DWORD __stdcall Spooler (LPVOID user);

    TexLoadRequest* getNextJob   (void)
    {
      TexLoadRequest* job       = nullptr;
      DWORD           dwResults = 0;

      dwResults =
        SK_WaitForSingleObject (events_.jobs_added, 0UL);

      while (dwResults != WAIT_OBJECT_0)
      {
        dwResults =
          SK_WaitForSingleObject (events_.jobs_added, 0);
      }
      
      if (0 == ReadAcquire (&jobs_waiting_))
        return nullptr;

      SK::D3D9::TexLoadRef ref (nullptr);

      while (! jobs_.empty ())
      {    if (jobs_.try_pop (ref)) break; }

      job = ref;

      InterlockedDecrement (&jobs_waiting_);
      ResetEvent       (events_.jobs_added);

      return job;
    }

    friend class SK::D3D9::TextureManager;

    void            postFinished (TexLoadRequest* finished)
    {
      if (finished == nullptr) // WTF?
        return;

      std::scoped_lock <SK_Thread_HybridSpinlock>
                 _lock (cs_results_);

      SK_ReleaseAssert (finished        != nullptr &&
                        finished->pDest != nullptr);

      if (finished->pDest != nullptr)
      {
        // Remove the temporary reference we added earlier
        finished->pDest->Release ();

        results_.push      (finished);
        SetEvent           (events_.results_waiting);
      }
      InterlockedIncrement (&jobs_done_);
    }

  private:
    volatile LONG                       jobs_waiting_ = 0L;
    volatile LONG                       jobs_done_    = 0L;

    concurrent_queue <TexLoadRef>       jobs_;
    concurrent_queue <TexLoadRef>       results_;

    std::vector <
      std::unique_ptr <
        TextureWorkerThread
      >
    >                                   workers_;

    struct {
      HANDLE jobs_added;
      HANDLE results_waiting;
      HANDLE shutdown;
    } events_;

    //SK_Thread_HybridSpinlock cs_jobs_    =
    //  SK_Thread_HybridSpinlock ( 100UL);
    SK_Thread_HybridSpinlock cs_results_ =
      SK_Thread_HybridSpinlock (1000UL);

    HANDLE spool_thread_;

    bool init_worker_sync_ = false;
  };//extern *resample_pool;

  //
  // Split stream jobs into small and large in order to prevent
  //   starvation from wreaking havoc on load times.
  //
  //   This is a simple, but remarkably effective approach and
  //     further optimization work probably will not be done.
  //
  struct StreamSplitter
  {
    bool working (void) const noexcept
    {
      if (lrg_tex && lrg_tex->working ()) return true;
      if ( sm_tex &&  sm_tex->working ()) return true;

      return false;
    }

    size_t queueLength (void) const noexcept
    {
      size_t len = 0;

      if (lrg_tex) len += lrg_tex->queueLength ();
      if ( sm_tex) len +=  sm_tex->queueLength ();

      return len;
    }

    void getFinished (std::vector <TexLoadRequest *>& results) const
    {
      std::vector <TexLoadRequest *> lrg_loads;
      std::vector <TexLoadRequest *>  sm_loads;

      if ( sm_tex)  sm_tex->getFinished  (sm_loads);
      if (lrg_tex) lrg_tex->getFinished (lrg_loads);

      for ( auto& load : lrg_loads ) results.emplace_back (load);
      for ( auto& load :  sm_loads ) results.emplace_back (load);

      return;
    }

    void postJob (TexLoadRequest* job) const
    {
      assert ( lrg_tex != nullptr &&
                sm_tex != nullptr );

      // A "Large" load is one >= 128 KiB
      if (job->SrcDataSize > (128 * 1024))
          lrg_tex->postJob (job);
      else sm_tex->postJob (job);
    }

    std::unique_ptr <TextureThreadPool> lrg_tex;
    std::unique_ptr <TextureThreadPool> sm_tex;
  };
}
}

#include <SpecialK/utility.h>

#pragma comment (lib, "dxguid.lib")

const GUID IID_SKTextureD3D9 =
  { 0xace1f81b, 0x5f3f, 0x45f4, 0xbf, 0x9f, 0x1b, 0xaf, 0xdf, 0xba, 0x11, 0x9b };


struct SK_D3D9_TextureStorageBase
{
//SK::D3D9::TextureThreadPool *SK::D3D9::resample_pool = nullptr;
  SK::D3D9::StreamSplitter                                _stream_pool;

  // Cleanup
  std::queue <std::wstring>                               _screenshots_to_delete;

  // Textures that are missing mipmaps
  std::set   <IDirect3DBaseTexture9 *>                    _incomplete_textures;

  Concurrency::
  concurrent_unordered_map <uint32_t, IDirect3DTexture9*> _injected_textures;
  Concurrency::
  concurrent_unordered_map <uint32_t, float>              _injected_load_times;
  Concurrency::
  concurrent_unordered_map <uint32_t, size_t>             _injected_sizes;
  Concurrency::
  concurrent_unordered_map <uint32_t, int32_t>            _injected_refs;

  SK_ThreadSafe_HashSet <IDirect3DSurface9 *>             _outstanding_screenshots;
                                           // Not excellent screenshots, but screenhots
                                           //   that aren't finished yet and we can't reset
                                           //     the D3D9 device because of.
};

SK::D3D9::TextureManager&
SK_D3D9_GetTextureManager (void);

SK_D3D9_TextureStorageBase&
SK_D3D9_GetBasicTextureDataStore (void);

#define screenshots_to_delete   SK_D3D9_GetBasicTextureDataStore ()._screenshots_to_delete
#define incomplete_textures     SK_D3D9_GetBasicTextureDataStore ()._incomplete_textures
#define injected_textures       SK_D3D9_GetBasicTextureDataStore ()._injected_textures
#define injected_load_times     SK_D3D9_GetBasicTextureDataStore ()._injected_load_times
#define injected_sizes          SK_D3D9_GetBasicTextureDataStore ()._injected_sizes
#define injected_refs           SK_D3D9_GetBasicTextureDataStore ()._injected_refs
#define outstanding_screenshots SK_D3D9_GetBasicTextureDataStore ()._outstanding_screenshots
#define stream_pool             SK_D3D9_GetBasicTextureDataStore ()._stream_pool


interface ISKTextureD3D9 : public IDirect3DTexture9
{
public:
  enum class ContentPreference
  {
    DontCare = 0, // Follow the remap setting
    Original = 1,
    Override = 2
  };

  virtual ~ISKTextureD3D9 (void) { };

  ISKTextureD3D9 (IDirect3DTexture9 **ppTex, SIZE_T size, uint32_t crc32c)
  {
      pTexOverride  = nullptr;
      can_free      = false;
      override_size = 0;
      last_used.QuadPart
                    = 0ULL;
      pTex          = *ppTex;
    *ppTex          =  this;
      tex_size      = size;
      tex_crc32c    = crc32c;
      must_block    = false;
      uses          =  0;
      freed         = false;

      InterlockedExchange (&refs, 1);

      img_to_use    = ContentPreference::DontCare;
  };

  /*** IUnknown methods ***/
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) override
  {
    if (IsEqualGUID (riid, IID_SKTextureD3D9))
    {
      return S_OK;
    }

    if ( IsEqualGUID (riid, IID_IUnknown)              != FALSE ||
         IsEqualGUID (riid, IID_IDirect3DResource9)    != FALSE ||
         IsEqualGUID (riid, IID_IDirect3DTexture9)     != FALSE ||
         IsEqualGUID (riid, IID_IDirect3DBaseTexture9) != FALSE    )
    {
      AddRef ();

      *ppvObj = this;

      return S_OK;
    }

    return E_NOINTERFACE;
  }
  STDMETHOD_(ULONG,AddRef)(THIS)  override {
    InterlockedIncrement (&refs);

    can_free = false;

    return refs;
  }
  STDMETHOD_(ULONG,Release)(THIS) override {
    const ULONG ret = InterlockedDecrement (&refs);

    if (ret == 1)
    {
      can_free = true;
    }

    if (ret == 0)
    {
    //SK_ReleaseAssert (can_free);

      if ( pTex != nullptr &&
           pTex != this )
      {
        LPVOID                                                 dontcare;
        if ( FAILED (pTex->QueryInterface (IID_SKTextureD3D9, &dontcare)) &&
                                  std::exchange (freed, true) == false )
        {
          std::exchange (pTex, nullptr)->Release ();

          if (std::exchange (pTexOverride, nullptr) != nullptr)
          {
            if ( injected_textures.count (tex_crc32c)            &&
                 injected_textures.at    (tex_crc32c) != nullptr &&
                 injected_refs.count     (tex_crc32c) )
                 injected_refs           [tex_crc32c]--;
          }
        }
      }

      can_free = true;

      // Does not delete this immediately; defers the
      //   process until the next cached texture load.
      SK_D3D9_GetTextureManager ().removeTexture (this);
    }

    return ret;
  }

  /*** IDirect3DBaseTexture9 methods ***/
  STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice)  override {
    if (config.system.log_level > 1)
    {
      tex_log->Log (L"[ Tex. Mgr ] ISKTextureD3D9::GetDevice (%ph)", ppDevice);
    }

    if (pTex == nullptr)
      return E_FAIL;

    return pTex->GetDevice (ppDevice);
  }
  STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags)  override
  {
    if (config.system.log_level > 1)
    {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::SetPrivateData (%x, %ph, %lu, %x)",
                       refguid.Data1,
                         pData,
                           SizeOfData,
                             Flags );
    }

    if (pTex == nullptr)
      return S_OK;

    return pTex->SetPrivateData (refguid, pData, SizeOfData, Flags);
  }
  STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid,void* pData,DWORD* pSizeOfData)  override
  {
    if (config.system.log_level > 1)
    {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetPrivateData (%x, %ph, %lu)",
                       refguid.Data1,
                         pData,
                           *pSizeOfData );
    }

    if (pTex == nullptr)
      return S_OK;

    return pTex->GetPrivateData (refguid, pData, pSizeOfData);
  }
  STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid)  override {
    tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::FreePrivateData (%x)",
                     refguid.Data1 );

    if (pTex == nullptr)
      return E_FAIL;

    return pTex->FreePrivateData (refguid);
  }
  STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew)  override {
    tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::SetPriority (%lu)",
                     PriorityNew );

    if (pTex == nullptr)
      return 0;

    return pTex->SetPriority (PriorityNew);
  }
  STDMETHOD_(DWORD, GetPriority)(THIS)  override {
    tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetPriority ()" );

    if (pTex == nullptr)
      return 0;

    return pTex->GetPriority ();
  }
  STDMETHOD_(void, PreLoad)(THIS)  override {
    tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::PreLoad ()" );

    if (pTex == nullptr)
      return;

    pTex->PreLoad ();
  }
  STDMETHOD_(D3DRESOURCETYPE, GetType)(THIS)  override {
    if (config.system.log_level > 1)
    {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetType ()" );
    }

    if (pTex == nullptr)
      return D3DRTYPE_TEXTURE;

    return pTex->GetType ();
  }
  STDMETHOD_(DWORD, SetLOD)(THIS_ DWORD LODNew)  override {
    tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::SetLOD (%lu)",
                     LODNew );

    if (pTex == nullptr)
      return 0;

    return pTex->SetLOD (LODNew);
  }
  STDMETHOD_(DWORD, GetLOD)(THIS)  override {
    tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetLOD ()" );

    if (pTex == nullptr)
      return 0;

    return pTex->GetLOD ();
  }
  STDMETHOD_(DWORD, GetLevelCount)(THIS)  override
  {
    if (config.system.log_level > 1)
    {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetLevelCount ()" );
    }

    if (pTex == nullptr)
      return 0;

    return pTex->GetLevelCount ();
  }
  STDMETHOD(SetAutoGenFilterType)(THIS_ D3DTEXTUREFILTERTYPE FilterType)  override {
    tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::SetAutoGenFilterType (%x)",
                    FilterType );

    if (pTex == nullptr)
      return S_OK;

    return pTex->SetAutoGenFilterType (FilterType);
  }
  STDMETHOD_(D3DTEXTUREFILTERTYPE, GetAutoGenFilterType)(THIS)  override {
    tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetAutoGenFilterType ()" );

    if (pTex == nullptr)
      return D3DTEXF_POINT;

    return pTex->GetAutoGenFilterType ();
  }
  STDMETHOD_(void, GenerateMipSubLevels)(THIS)  override {
    tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GenerateMipSubLevels ()" );

    if (pTex == nullptr)
      return;

    pTex->GenerateMipSubLevels ();
  }
  STDMETHOD(GetLevelDesc)(THIS_ UINT Level,D3DSURFACE_DESC *pDesc)  override
  {
    if (config.system.log_level > 1)
    {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetLevelDesc (%lu, %ph)",
                     Level,
                       pDesc );
    }

    if (pTex == nullptr)
      return S_OK;

    return pTex->GetLevelDesc (Level, pDesc);
  }
  STDMETHOD(GetSurfaceLevel)(THIS_ UINT Level,IDirect3DSurface9** ppSurfaceLevel)  override
  {
    if (config.system.log_level > 1)
    {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetSurfaceLevel (%lu, %ph)",
                      Level,
                        ppSurfaceLevel );
    }

    if (pTex == nullptr)
      return S_OK;

    return pTex->GetSurfaceLevel (Level, ppSurfaceLevel);
  }
  STDMETHOD(LockRect)(THIS_ UINT Level,D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags)  override
  {
    if (config.system.log_level > 1)
    {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::LockRect (%lu, %ph, %ph, %x)",
                       Level,
                         pLockedRect,
                           pRect,
                             Flags );
    }

    if (pTex == nullptr)
      return S_OK;

    const HRESULT hr =
      pTex->LockRect (Level, pLockedRect, pRect, Flags);

    if (SUCCEEDED (hr))
    {
      if (Level == 0)
      {
        lock_lvl0 = *pLockedRect;
        dirty     = true;
        begin_map = SK_QueryPerf ();
      }
    }

    return hr;
  }
  STDMETHOD(UnlockRect)(THIS_ UINT Level);
  STDMETHOD(AddDirtyRect)(THIS_ CONST RECT* pDirtyRect)  override
  {
    if (config.system.log_level > 1)
    {
      tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::AddDirtyRect (...)" );
    }

    if (pTex == nullptr)
      return S_OK;

    const HRESULT hr =
      pTex->AddDirtyRect (pDirtyRect);

    if (SUCCEEDED (hr))
    {
      dirty = true;
    }

    return hr;
  }

  IDirect3DTexture9* getDrawTexture (void) const;
  IDirect3DTexture9* use            (void);

  void               toggleOverride (void);
  void               toggleOriginal (void);

  bool               can_free;      // Whether or not we can free this texture
  bool               must_block;    // Whether or not to draw using this texture before its
                                    //  override finishes streaming

  bool               dirty  = false;//   If the game has changed this texture
  bool               freed  = false;

  IDirect3DTexture9* pTex;          // The original texture data
  SSIZE_T            tex_size;      //   Original data size
  uint32_t           tex_crc32c;    //   Original data checksum

  IDirect3DTexture9* pTexOverride;  // The overridden texture data (nullptr if unchanged)
  SSIZE_T            override_size; //   Override data size

  volatile LONG      refs;
  LARGE_INTEGER      last_used;     // The last time this texture was used (for rendering)
                                    //   different from the last time referenced, this is
                                    //     set when SetTexture (...) is called.
                                    //
                                    //

  D3DLOCKED_RECT     lock_lvl0  = {      };
  int                uses       =      0;
  LARGE_INTEGER      begin_map  = { 0, 0 };
  LARGE_INTEGER      end_map    = { 0, 0 };

  ContentPreference  img_to_use;
};

typedef HRESULT (STDMETHODCALLTYPE *D3DXCreateTextureFromFileInMemoryEx_pfn)
(
  _In_        LPDIRECT3DDEVICE9  pDevice,
  _In_        LPCVOID            pSrcData,
  _In_        UINT               SrcDataSize,
  _In_        UINT               Width,
  _In_        UINT               Height,
  _In_        UINT               MipLevels,
  _In_        DWORD              Usage,
  _In_        D3DFORMAT          Format,
  _In_        D3DPOOL            Pool,
  _In_        DWORD              Filter,
  _In_        DWORD              MipFilter,
  _In_        D3DCOLOR           ColorKey,
  _Inout_opt_ D3DXIMAGE_INFO     *pSrcInfo,
  _Out_opt_   PALETTEENTRY       *pPalette,
  _Out_       LPDIRECT3DTEXTURE9 *ppTexture
);

typedef HRESULT (STDMETHODCALLTYPE *D3DXCreateTextureFromFileExW_pfn)
(
  _In_    LPDIRECT3DDEVICE9  pDevice,
  _In_    LPCWSTR            pSrcFile,
  _In_    UINT               Width,
  _In_    UINT               Height,
  _In_    UINT               MipLevels,
  _In_    DWORD              Usage,
  _In_    D3DFORMAT          Format,
  _In_    D3DPOOL            Pool,
  _In_    DWORD              Filter,
  _In_    DWORD              MipFilter,
  _In_    D3DCOLOR           ColorKey,
  _Inout_ D3DXIMAGE_INFO     *pSrcInfo,
  _Out_   PALETTEENTRY       *pPalette,
  _Out_   LPDIRECT3DTEXTURE9 *ppTexture
);

typedef HRESULT (STDMETHODCALLTYPE *D3DXCreateVolumeTextureFromFileInMemoryEx_pfn)
(
  _In_    LPDIRECT3DDEVICE9        pDevice,
  _In_    LPCVOID                  pSrcData,
  _In_    UINT                     SrcDataSize,
  _In_    UINT                     Width,
  _In_    UINT                     Height,
  _In_    UINT                     Depth,
  _In_    UINT                     MipLevels,
  _In_    DWORD                    Usage,
  _In_    D3DFORMAT                Format,
  _In_    D3DPOOL                  Pool,
  _In_    DWORD                    Filter,
  _In_    DWORD                    MipFilter,
  _In_    D3DCOLOR                 ColorKey,
  _Inout_ D3DXIMAGE_INFO           *pSrcInfo,
  _Out_   PALETTEENTRY             *pPalette,
  _Out_   LPDIRECT3DVOLUMETEXTURE9 *ppVolumeTexture
);

typedef HRESULT (STDMETHODCALLTYPE *D3DXCreateCubeTextureFromFileInMemoryEx_pfn)
(
  _In_    LPDIRECT3DDEVICE9      pDevice,
  _In_    LPCVOID                pSrcData,
  _In_    UINT                   SrcDataSize,
  _In_    UINT                   Size,
  _In_    UINT                   MipLevels,
  _In_    DWORD                  Usage,
  _In_    D3DFORMAT              Format,
  _In_    D3DPOOL                Pool,
  _In_    DWORD                  Filter,
  _In_    DWORD                  MipFilter,
  _In_    D3DCOLOR               ColorKey,
  _Inout_ D3DXIMAGE_INFO         *pSrcInfo,
  _Out_   PALETTEENTRY           *pPalette,
  _Out_   LPDIRECT3DCUBETEXTURE9 *ppCubeTexture
);

typedef HRESULT (STDMETHODCALLTYPE *D3DXCreateTextureFromFileExA_pfn)
(
  _In_    LPDIRECT3DDEVICE9  pDevice,
  _In_    LPCSTR             pSrcFile,
  _In_    UINT               Width,
  _In_    UINT               Height,
  _In_    UINT               MipLevels,
  _In_    DWORD              Usage,
  _In_    D3DFORMAT          Format,
  _In_    D3DPOOL            Pool,
  _In_    DWORD              Filter,
  _In_    DWORD              MipFilter,
  _In_    D3DCOLOR           ColorKey,
  _Inout_ D3DXIMAGE_INFO     *pSrcInfo,
  _Out_   PALETTEENTRY       *pPalette,
  _Out_   LPDIRECT3DTEXTURE9 *ppTexture
);

typedef HRESULT (STDMETHODCALLTYPE *D3DXSaveTextureToFile_pfn)(
  _In_           LPCWSTR                 pDestFile,
  _In_           D3DXIMAGE_FILEFORMAT    DestFormat,
  _In_           LPDIRECT3DBASETEXTURE9  pSrcTexture,
  _In_opt_ const PALETTEENTRY           *pSrcPalette
);

typedef HRESULT (WINAPI *D3DXSaveSurfaceToFile_pfn)
(
  _In_           LPCWSTR               pDestFile,
  _In_           D3DXIMAGE_FILEFORMAT  DestFormat,
  _In_           LPDIRECT3DSURFACE9    pSrcSurface,
  _In_opt_ const PALETTEENTRY         *pSrcPalette,
  _In_opt_ const RECT                 *pSrcRect
);

typedef HRESULT (STDMETHODCALLTYPE *CreateTexture_pfn)
(
  IDirect3DDevice9   *This,
  UINT                Width,
  UINT                Height,
  UINT                Levels,
  DWORD               Usage,
  D3DFORMAT           Format,
  D3DPOOL             Pool,
  IDirect3DTexture9 **ppTexture,
  HANDLE             *pSharedHandle
);

typedef HRESULT (STDMETHODCALLTYPE *CreateRenderTarget_pfn)
(
  IDirect3DDevice9     *This,
  UINT                  Width,
  UINT                  Height,
  D3DFORMAT             Format,
  D3DMULTISAMPLE_TYPE   MultiSample,
  DWORD                 MultisampleQuality,
  BOOL                  Lockable,
  IDirect3DSurface9   **ppSurface,
  HANDLE               *pSharedHandle
);

typedef HRESULT (STDMETHODCALLTYPE *CreateDepthStencilSurface_pfn)
(
  IDirect3DDevice9     *This,
  UINT                  Width,
  UINT                  Height,
  D3DFORMAT             Format,
  D3DMULTISAMPLE_TYPE   MultiSample,
  DWORD                 MultisampleQuality,
  BOOL                  Discard,
  IDirect3DSurface9   **ppSurface,
  HANDLE               *pSharedHandle
);

typedef HRESULT (STDMETHODCALLTYPE *SetTexture_pfn)(
  _In_ IDirect3DDevice9      *This,
  _In_ DWORD                  Sampler,
  _In_ IDirect3DBaseTexture9 *pTexture
);

typedef HRESULT (STDMETHODCALLTYPE *SetRenderTarget_pfn)(
  _In_ IDirect3DDevice9      *This,
  _In_ DWORD                  RenderTargetIndex,
  _In_ IDirect3DSurface9     *pRenderTarget
);

typedef HRESULT (STDMETHODCALLTYPE *SetDepthStencilSurface_pfn)(
  _In_ IDirect3DDevice9      *This,
  _In_ IDirect3DSurface9     *pNewZStencil
);
