#ifndef INTERFACE
#define INTERFACE


#define DEFAULT_MAX_CONNECTIONS             10

#define DEFAULT_SERVER_LISTEN_PATH          "/tmp/mcw_server"
#define DEFAULT_SERVER_LISTEN_ROOT_PATH     "/tmp/mcw_server_root"


#define DEFAULT_REQUEST_CID                 "CID"
#define DEFAULT_REQUEST_LIST_DIR            "LD"
#define DEFAULT_REQUEST_SCAN_DIR            "SD"
#define DEFAULT_REQUEST_TREE_DIR            "TD"
#define DEFAULT_REQUEST_MAKE_DIR            "MD"
#define DEFAULT_REQUEST_DELETE_FILE         "DEL"
#define DEFAULT_REQUEST_SEARCH_FILE         "SRCH"
#define DEFAULT_REQUEST_COPY_FILE           "COPY"
#define DEFAULT_REQUEST_MOVE_FILE           "MOVE"

#define DEFAULT_REQUEST_ABORT               "ABORT"
#define DEFAULT_REQUEST_QUIT                "QUIT"


#define DEFAULT_REQUEST_OK                  QDialogButtonBox::Ok
#define DEFAULT_REQUEST_CANCEL              QDialogButtonBox::Cancel
#define DEFAULT_REQUEST_YES                 QDialogButtonBox::Yes
#define DEFAULT_REQUEST_YESALL              QDialogButtonBox::YesToAll
#define DEFAULT_REQUEST_NO                  QDialogButtonBox::No
#define DEFAULT_REQUEST_NOALL               QDialogButtonBox::NoToAll
#define DEFAULT_REQUEST_SKIP                QDialogButtonBox::Ignore
#define DEFAULT_REQUEST_SKIPALL            (QDialogButtonBox::Ignore + 1)
#define DEFAULT_REQUEST_RETRY               QDialogButtonBox::Retry



#define DEFAULT_RESPONSE_CID                "CID"
#define DEFAULT_RESPONSE_QUEUE              "QUEUE"


#define DEFAULT_RESPONSE_OK                 0
#define DEFAULT_RESPONSE_EXISTS             1
#define DEFAULT_RESPONSE_ERROR              2
#define DEFAULT_RESPONSE_NON_EMPTY          3
#define DEFAULT_RESPONSE_HIDDEN             4





#endif // INTERFACE

