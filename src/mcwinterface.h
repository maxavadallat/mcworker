#ifndef INTERFACE
#define INTERFACE

// Max Number Of Client Connections
#define DEFAULT_MAX_CONNECTIONS                 10

// File Server Worker App Name
#define DEFAULT_FILE_SERVER_EXEC_NAME           "mcworker"

// Listen Path
#define DEFAULT_SERVER_LISTEN_PATH              "/tmp/mcw_server"
#define DEFAULT_SERVER_LISTEN_ROOT_PATH         "/tmp/mcw_server_root"

// Root Option Command Line Parameter
#define DEFAULT_OPTION_RUNASROOT                "--root"

// Data Map Keys
#define DEFAULT_KEY_CID                         "cid"
#define DEFAULT_KEY_OPERATION                   "op"
#define DEFAULT_KEY_FILENAME                    "fn"
#define DEFAULT_KEY_CURRPROGRESS                "cp"
#define DEFAULT_KEY_CURRTOTAL                   "ct"
#define DEFAULT_KEY_OVERALLPROGRESS             "op"
#define DEFAULT_KEY_OVERALLTOTAL                "ot"
#define DEFAULT_KEY_SPEED                       "spd"
#define DEFAULT_KEY_SOURCE                      "src"
#define DEFAULT_KEY_TARGET                      "trg"
#define DEFAULT_KEY_PATH                        "path"
#define DEFAULT_KEY_FILTERS                     "fltrs"
#define DEFAULT_KEY_OPTIONS                     "opt"
#define DEFAULT_KEY_FLAGS                       "flgs"
#define DEFAULT_KEY_CONTENT                     "cntnt"
#define DEFAULT_KEY_ERROR                       "error"
#define DEFAULT_KEY_NUMFILES                    "nf"
#define DEFAULT_KEY_NUMDIRS                     "nd"
#define DEFAULT_KEY_DIRSIZE                     "ds"
#define DEFAULT_KEY_RESPONSE                    "resp"
#define DEFAULT_KEY_CONFIRMCODE                 "conf"
#define DEFAULT_KEY_READY                       "rdy"


// Operation Codes
#define DEFAULT_OPERATION_LIST_DIR              "LD"
#define DEFAULT_OPERATION_SCAN_DIR              "SD"
#define DEFAULT_OPERATION_TREE_DIR              "TD"
#define DEFAULT_OPERATION_MAKE_DIR              "MD"
#define DEFAULT_OPERATION_DELETE_FILE           "DEL"
#define DEFAULT_OPERATION_SEARCH_FILE           "SRCH"
#define DEFAULT_OPERATION_COPY_FILE             "CPY"
#define DEFAULT_OPERATION_MOVE_FILE             "MV"
#define DEFAULT_OPERATION_PERMISSIONS           "PERM"
#define DEFAULT_OPERATION_ATTRIBUTES            "ATTR"
#define DEFAULT_OPERATION_OWNER                 "OWNR"
#define DEFAULT_OPERATION_ABORT                 "ABORT"
#define DEFAULT_OPERATION_QUIT                  "QUIT"
#define DEFAULT_OPERATION_RESP                  "RESP"
#define DEFAULT_OPERATION_ACKNOWLEDGE           "ACKN"

#define DEFAULT_OPERATION_TEST                  "TEST"

// Response Values
#define DEFAULT_RESPONSE_OK                     QDialogButtonBox::Ok
#define DEFAULT_RESPONSE_CANCEL                 QDialogButtonBox::Cancel
#define DEFAULT_RESPONSE_YES                    QDialogButtonBox::Yes
#define DEFAULT_RESPONSE_YESALL                 QDialogButtonBox::YesToAll
#define DEFAULT_RESPONSE_NO                     QDialogButtonBox::No
#define DEFAULT_RESPONSE_NOALL                  QDialogButtonBox::NoToAll
#define DEFAULT_RESPONSE_SKIP                   QDialogButtonBox::Ignore
#define DEFAULT_RESPONSE_SKIPALL               (QDialogButtonBox::Ignore + 1)
#define DEFAULT_RESPONSE_RETRY                  QDialogButtonBox::Retry


// Response Codes
#define DEFAULT_RESPONSE_CID                    "CID"
#define DEFAULT_RESPONSE_DIRITEM                "DLI"
#define DEFAULT_RESPONSE_DIRSCAN                "DSC"
#define DEFAULT_RESPONSE_DIRTREE                "DTR"
#define DEFAULT_RESPONSE_QUEUE                  "QLI"
#define DEFAULT_RESPONSE_CONFIRM                "CNF"
#define DEFAULT_RESPONSE_PROGRESS               "PRG"
#define DEFAULT_RESPONSE_READY                  "RDY"
#define DEFAULT_RESPONSE_ERROR                  "ERR"

// Error Codes
#define DEFAULT_ERROR_OK                        0x0000
#define DEFAULT_ERROR_GENERAL                   0x0001
#define DEFAULT_ERROR_EXISTS                    0x0002
#define DEFAULT_ERROR_NOTEXISTS                 0x0003
#define DEFAULT_ERROR_NON_EMPTY                 0x0004
#define DEFAULT_ERROR_HIDDEN                    0x0005
#define DEFAULT_ERROR_SYSTEM                    0x0006
#define DEFAULT_ERROR_ACCESS                    0x0007


// Sort Flags
#define DEFAULT_SORT_NAME                       0x0000
#define DEFAULT_SORT_EXT                        0x0001
#define DEFAULT_SORT_TYPE                       0x0002
#define DEFAULT_SORT_SIZE                       0x0003
#define DEFAULT_SORT_DATE                       0x0004
#define DEFAULT_SORT_OWNER                      0x0005
#define DEFAULT_SORT_PERMS                      0x0006
#define DEFAULT_SORT_ATTR                       0x0007

#define DEFAULT_SORT_REVERSE                    0x0100

#define DEFAULT_SORT_DIRFIRST                   0x1000

#define DEFAULT_SORT_CASE                       0x2000


// Filter Options
#define DEFAULT_FILTER_SHOW_HIDDEN              0x0001



// Default Root File Server Idle Timeout in Millisecs
#define DEFAULT_ROOT_FILE_SERVER_IDLE_TIMEOUT   1000

// Default Root File Server Idle Max Ticks - 5 MINUTES
#define DEFAULT_ROOT_FILE_SERVER_IDLE_MAX_TICKS (5 * 60)




#endif // INTERFACE

