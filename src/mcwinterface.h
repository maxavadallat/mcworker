#ifndef INTERFACE
#define INTERFACE

// Max Number Of Client Connections
#define DEFAULT_MAX_CONNECTIONS                 10

// File Server Worker App Name
#define DEFAULT_FILE_SERVER_EXEC_NAME           "mcworker"

// File Server Host Port
#define DEFAULT_FILE_SERVER_HOST_PORT           7707
#define DEFAULT_FILE_SERVER_ROOT_HOST_PORT      7708

// Listen Path
#define DEFAULT_SERVER_LISTEN_PATH              "/tmp/mcw_server"
#define DEFAULT_SERVER_LISTEN_ROOT_PATH         "/tmp/mcw_server_root"

// Root Option Command Line Parameter
#define DEFAULT_OPTION_RUNASROOT                "--root"

// Default Data Frame Pattern
#define DEFAULT_DATA_FRAME_PATTERN_CHAR_1       '\x07'
#define DEFAULT_DATA_FRAME_PATTERN_CHAR_2       '\x07'
#define DEFAULT_DATA_FRAME_PATTERN_CHAR_3       '\x00'
#define DEFAULT_DATA_FRAME_PATTERN_CHAR_4       '\x07'

// Data Map Keys
#define DEFAULT_KEY_CID                         "cid"
#define DEFAULT_KEY_OPERATION                   "op"
#define DEFAULT_KEY_FILENAME                    "fn"
#define DEFAULT_KEY_CURRPROGRESS                "cp"
#define DEFAULT_KEY_CURRTOTAL                   "ct"
#define DEFAULT_KEY_SOURCE                      "src"
#define DEFAULT_KEY_TARGET                      "trg"
#define DEFAULT_KEY_PATH                        "path"
#define DEFAULT_KEY_FILTERS                     "fltrs"
#define DEFAULT_KEY_OPTIONS                     "opt"
#define DEFAULT_KEY_FLAGS                       "flgs"
#define DEFAULT_KEY_PERMISSIONS                 "prms"
#define DEFAULT_KEY_OWNER                       "ownr"
#define DEFAULT_KEY_ATTRIB                      "attr"
#define DEFAULT_KEY_DATETIME                    "date"
#define DEFAULT_KEY_CONTENT                     "cntnt"
#define DEFAULT_KEY_ERROR                       "error"
#define DEFAULT_KEY_NUMFILES                    "nf"
#define DEFAULT_KEY_NUMDIRS                     "nd"
#define DEFAULT_KEY_DIRSIZE                     "ds"
#define DEFAULT_KEY_RESPONSE                    "resp"
#define DEFAULT_KEY_CONFIRMCODE                 "conf"
#define DEFAULT_KEY_READY                       "rdy"
#define DEFAULT_KEY_CUSTOM                      "user"


// Operation Codes
#define DEFAULT_OPERATION_LIST_DIR              "LD"
#define DEFAULT_OPERATION_SCAN_DIR              "SD"
#define DEFAULT_OPERATION_TREE_DIR              "TD"
#define DEFAULT_OPERATION_MAKE_DIR              "MD"
#define DEFAULT_OPERATION_MAKE_LINK             "ML"
#define DEFAULT_OPERATION_DELETE_FILE           "DEL"
#define DEFAULT_OPERATION_QUEUE                 "QUE"
#define DEFAULT_OPERATION_SEARCH_FILE           "SRCH"
#define DEFAULT_OPERATION_COPY_FILE             "CPY"
#define DEFAULT_OPERATION_MOVE_FILE             "MV"
#define DEFAULT_OPERATION_PERMISSIONS           "PERM"
#define DEFAULT_OPERATION_ATTRIBUTES            "ATTR"
#define DEFAULT_OPERATION_OWNER                 "OWNR"
#define DEFAULT_OPERATION_DATETIME              "DATE"
#define DEFAULT_OPERATION_ABORT                 "ABORT"
#define DEFAULT_OPERATION_QUIT                  "QUIT"
#define DEFAULT_OPERATION_USER_RESP             "RESP"
#define DEFAULT_OPERATION_ACKNOWLEDGE           "ACKN"
#define DEFAULT_OPERATION_PAUSE                 "PSE"
#define DEFAULT_OPERATION_RESUME                "RSM"
#define DEFAULT_OPERATION_CONTENT               "CNT"
#define DEFAULT_OPERATION_CLEAR                 "CLR"

#define DEFAULT_OPERATION_TEST                  "TEST"

// Confirm Values
#define DEFAULT_CONFIRM_OK                      QDialogButtonBox::Ok
#define DEFAULT_CONFIRM_CANCEL                  QDialogButtonBox::Cancel
#define DEFAULT_CONFIRM_YES                     QDialogButtonBox::Yes
#define DEFAULT_CONFIRM_YESALL                  QDialogButtonBox::YesToAll
#define DEFAULT_CONFIRM_NO                      QDialogButtonBox::No
#define DEFAULT_CONFIRM_NOALL                   QDialogButtonBox::NoToAll
#define DEFAULT_CONFIRM_SKIP                    QDialogButtonBox::Ignore
#define DEFAULT_CONFIRM_SKIPALL                (QDialogButtonBox::Ignore + 1)
#define DEFAULT_CONFIRM_RETRY                   QDialogButtonBox::Retry
#define DEFAULT_CONFIRM_ABORT                   QDialogButtonBox::Abort


// Response Codes
#define DEFAULT_RESPONSE_CID                    "CID"
#define DEFAULT_RESPONSE_DIRITEM                "DLI"
#define DEFAULT_RESPONSE_DIRSCAN                "DSC"
#define DEFAULT_RESPONSE_DIRTREE                "DTR"
#define DEFAULT_RESPONSE_SEARCH                 "SRCH"
#define DEFAULT_RESPONSE_QUEUE                  "QLI"
#define DEFAULT_RESPONSE_START                  "STRT"
#define DEFAULT_RESPONSE_PROGRESS               "PRG"
#define DEFAULT_RESPONSE_CONFIRM                "CNF"
#define DEFAULT_RESPONSE_SKIP                   "SKP"
#define DEFAULT_RESPONSE_READY                  "RDY"
#define DEFAULT_RESPONSE_ABORT                  "ABRT"
#define DEFAULT_RESPONSE_ERROR                  "ERR"
#define DEFAULT_RESPONSE_ADMIN                  "ADM"


#define DEFAULT_RESPONSE_TEST                   "TEST"


// Error Codes
#define DEFAULT_ERROR_OK                        0x0000
#define DEFAULT_ERROR_GENERAL                   0x0001
#define DEFAULT_ERROR_EXISTS                    0x0002
#define DEFAULT_ERROR_NOTEXISTS                 0x0003
#define DEFAULT_ERROR_NON_EMPTY                 0x0004
#define DEFAULT_ERROR_HIDDEN                    0x0005
#define DEFAULT_ERROR_SYSTEM                    0x0006
#define DEFAULT_ERROR_ACCESS                    0x0007
#define DEFAULT_ERROR_DISKFULL                  0x0008
#define DEFAULT_ERROR_TARGET_DIR_NOT_EXISTS     0x0009
#define DEFAULT_ERROR_TARGET_DIR_EXISTS         0x000A
#define DEFAULT_ERROR_CANNOT_DELETE_SOURCE_DIR  0x000B
#define DEFAULT_ERROR_CANNOT_DELETE_TARGET_DIR  0x000C


// Sort Flags
#define DEFAULT_SORT_NAME                       0x0000
#define DEFAULT_SORT_EXT                        0x0001
#define DEFAULT_SORT_TYPE                       0x0002
#define DEFAULT_SORT_SIZE                       0x0003
#define DEFAULT_SORT_DATE                       0x0004
#define DEFAULT_SORT_OWNER                      0x0005
#define DEFAULT_SORT_PERMS                      0x0006
#define DEFAULT_SORT_ATTRS                      0x0007

#define DEFAULT_SORT_REVERSE                    0x0100
#define DEFAULT_SORT_DIRFIRST                   0x1000
#define DEFAULT_SORT_CASE                       0x2000

// Filter Options
#define DEFAULT_FILTER_SHOW_HIDDEN              0x0001

// Copy Options
#define DEFAULT_COPY_OPTIONS_COPY_HIDDEN        0x0001

// Search Options
#define DEFAULT_SEARCH_OPTION_CASE_SENSITIVE    0x0001
#define DEFAULT_SEARCH_OPTION_WHOLE_WORD        0x0010


// Default Root File Server Idle Timeout in Millisecs
#define DEFAULT_ROOT_FILE_SERVER_IDLE_TIMEOUT   1000

// Default Root File Server Idle Max Ticks - 5 MINUTES
#define DEFAULT_ROOT_FILE_SERVER_IDLE_MAX_TICKS (5 * 60)




#endif // INTERFACE

