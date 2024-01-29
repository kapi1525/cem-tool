#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include <cinttypes>



namespace fusion {
    // Supported platforms bitfield
    enum platform : std::uint32_t {
        windows = 1<<0,
        adobe_flash = 1<<1,
        android = 1<<2,
        ios = 1<<3,
        html = 1<<4,
        uwp = 1<<5,
        macos = 1<<6,
        xna = 1<<7,
        last = 1<<8
    };

    // Order matches platform enum
    static const char* platform_names[] = {
        "win",
        "swf",
        "android",
        "ios",
        "HTML5",
        "uwp",
        "macos",
        "xna"
    };

    // Convert between enum and platform_names array index.
    std::uint32_t platform_enum_to_index(platform platform_enum);
    platform platform_index_to_enum(std::uint32_t platform_index);

    // json file with extension info used by extension manager
    struct cem_ext_manifest {
        std::string mfxname;                // Extension mfx file name
        std::string name;
        std::string author;
        std::string description;
        std::string website;
        int dev;                            // Is developer version only?
        std::uint32_t platforms;            // Supported platforms (platform enum)
        std::string download;               // Download link or lower case mfxname if ext is stored on clickteams server
        time_t time;                        // Modification date and time of the most recent file inside the zip file, used by fusion for update checks
        std::uint32_t zipsize;              // Size of zip archive
        std::vector<std::string> files;     // List of all files inside zip archive

        std::string to_json();
    };


    // Thigs used to talk with fusion extensions with original names
    namespace api {
        // A lot of random aliases
        using HINSTANCE = void*;
        using npAppli = void*;
        using npWin = void*;
        using HWND = void*;
        using HPALETTE = void*;
        using CImageFilterMgr = void*;
        using CSoundFilterMgr = void*;
        using CSoundManager = void*;
        using CEditApp = void*;
        using CRunApp = void*;
        using CEditFrame = void*;
        using CRunFrame = void*;
        using dllTrans = char*;
        using HHOOK = char*;
        using LPTSTR = char*;
        using LPCWSTR = wchar_t*;

        class mv {
        public:
            // Common to editor and runtime
            HINSTANCE HInst; // Application HINSTANCE
            npAppli IdAppli; // Application object in DLL
            npWin IdMainWin; // Main window object in DLL
            npWin IdEditWin; // Child window object in DLL
            HWND HMainWin; // Main window handle
            HWND HEditWin; // Child window handle
            HPALETTE HPal256; // 256 color palette
            std::uint16_t AppMode; // Screen mode with flags
            std::uint16_t ScrMode; // Screen mode
            std::uint32_t EditDXDocToClient; // Edit time only: top-left coordinates
            std::uint32_t EditDYDocToClient;
            CImageFilterMgr* ImgFilterMgr; // Image filter manager
            CSoundFilterMgr* SndFilterMgr; // Sound filter manager
            CSoundManager* SndMgr; // Sound manager

            union {
                CEditApp* EditApp; // Current application, edit time (not used)
                CRunApp* RunApp; // Current application, runtime
            };
            union {
                CEditFrame* EditFrame;
                CRunFrame* RunFrame;
            };

            // Runtime
            void* RunHdr;                             // Current RunHeader
            std::uint32_t Prefs; // Preferences (sound on/off)
            LPTSTR subType;
            int FullScreen; // Full screen mode
            LPTSTR MainAppFileName; // App filename
            int AppListCount;
            int AppListSize;
            CRunApp** AppList;
            int ExtListCount;
            int ExtListSize;
            LPTSTR* ExtList;
            int NbDllTrans;
            dllTrans* DllTransList;
            std::uint32_t JoyCaps[32];
            HHOOK HMsgHook;
            int ModalLoop;
            int ModalSubAppCount;
            std::uint32_t LanguageID;
            LPCWSTR ModuleTextsPathname;
            void* Free[3];

            // Functions
            ////////////

            // Editor: Open Help file
            void* HelpA;

            // Editor: Get default font for object creation
            void* GetDefaultFontA;

            // Editor: Edit images and animations
            void* EditSurfaceA;
            void* EditImageA;
            void* EditAnimationA;

            // Runtime: Extension User data
            void* GetExtUserData;
            void* SetExtUserData;

            // Runtime: Register dialog box
            void* RegisterDialogBox;
            void* UnregisterDialogBox;

            // Runtime: Add surface as backdrop object
            void* AddBackdrop;

            // Runtime: Binary files
            void* GetFileA;
            void* ReleaseFileA;
            void* OpenHFileA;
            void* CloseHFile;

            // Plugin: download file
            void* LoadNetFileA;

            // Plugin: send command to Vitalize
            void* NetCommandA;

            // Editor & Runtime: Returns the version of MMF or of the runtime
            std::uint32_t (__stdcall* GetVersion)();

            // Editor & Runtime: callback function for properties or other functions
            void* CallFunction;

            // Editor: Open Help file (UNICODE)
            void* HelpW;

            // Editor: Get default font for object creation (UNICODE)
            void* GetDefaultFontW;

            // Editor: Edit images and animations (UNICODE)
            void* EditSurfaceW;
            void* EditImageW;
            void* EditAnimationW;

            // Runtime: Binary files (UNICODE
            void* GetFileW;
            void* ReleaseFileW;
            void* OpenHFileW;

            // Plugin: download file
            void* LoadNetFileW;

            // Plugin: send command to Vitalize
            void* NetCommandW;

            // Place-holder for next versions
            void* AdditionalFncs[6];
        };

        enum class KGI : int {
            version,
            not_used,
            plugin,
            multiple_sub_type,
            not_used2,
            atx_control,         // Not used
            product,
            build,
            unicode,
        };

        using OEFLAGS = std::uint32_t;
        using OEPREFS = std::uint16_t;

        struct kpxRunInfos {
            void* conditions;
            void* actions;
            void* expressions;
            short num_of_conditions;
            short num_of_actions;
            short num_of_expressions;
            std::uint16_t editdata_size;
            OEFLAGS edit_flags;
            std::uint8_t window_prop_priority;
            std::uint8_t free;
            OEPREFS edit_prefs;
            std::uint32_t identifier;
            short version;
        };

        // Some functions extensions export
        namespace ext_funcs {
            using Initialize = short (mv* mV, int quiet);
            using Free = int (mv* mV);
            using GetInfos = std::uint32_t (int info);
            using GetRunObjectInfos = short (mv* mV, kpxRunInfos* infoPtr);
            using GetObjInfosA = void (mv* mV, void* edPtr, char* ObjName, char* ObjAuthor, char* ObjCopyright, char* ObjComment, char* ObjHttp);
            using GetObjInfosW = void (mv* mV, void* edPtr, wchar_t* ObjName, wchar_t* ObjAuthor, wchar_t* ObjCopyright, wchar_t* ObjComment, wchar_t* ObjHttp);
        }
    }

    // More sane names
    using ext_general_infos = api::KGI;
    using ext_run_infos = api::kpxRunInfos;

    // in official sdk GetObjInfos() copies those strings to buffers passed as pointers
    struct ext_infos {
        std::string name;
        std::string author;
        std::string copyright;
        std::string comment;
        std::string website;
    };

    // Extension wrapper class.
    // Loads extension mfx and can call its functions
    // Some functions were modified to be more sane eg: GetObjInfos().
    // This class handles unicode and returns everything in utf8.
    class extension {
    public:
        extension() = default;
        ~extension() = default;

        void open(std::filesystem::path mfx_path);
        void close();

        bool is_open();

        short Initialize(int quiet);
        int Free();

        std::uint32_t GetInfos(ext_general_infos info);
        short GetRunObjectInfos(ext_run_infos* infos_ptr);
        void GetObjInfos(ext_infos* infos_ptr);

    private:
        // HMODULE
        void* module_handle = nullptr;

        bool is_unicode;

        api::mv dummy_mv = {};

        struct {
            std::function<api::ext_funcs::Initialize> Initialize;
            std::function<api::ext_funcs::Free> Free;
            std::function<api::ext_funcs::GetInfos> GetInfos;
            std::function<api::ext_funcs::GetRunObjectInfos> GetRunObjectInfos;
            std::function<api::ext_funcs::GetObjInfosA> GetObjInfosA;
            std::function<api::ext_funcs::GetObjInfosW> GetObjInfosW;
        } funcs;
    };
}