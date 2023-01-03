static char SQhdr_Accept_Charset[] = "Accept-Charset: ";
static char SQhdr_Accept_Encoding[] = "Accept-Encoding: ";
static char SQhdr_Accept_Language[] = "Accept-Language: ";
static char SQhdr_Accept[] = "Accept: ";
static char SQhdr_Authorization[] = "Authorization: ";
static char SQhdr_Connection_close[] = "Connection: close";
static char SQhdr_Connection_Keep_Alive[] = "Connection: Keep-Alive";
static char SQhdr_Date[] = "Date: ";
static char SQhdr_ETag[] = "ETag: ";
static char SQhdr_ETagW[] = "ETag: W/";
static char SQhdr_Host[] = "Host: ";
static char SQhdr_If_Modified_Since[] = "If-Modified-Since: ";
static char SQhdr_If_Unmodified_Since[] = "If-Unmodified-Since: ";
static char SQhdr_Keep_Alive[] = "Keep-Alive: ";
static char SQhdr_Pragma_no_cache[] = "Pragma: no-cache";
static char SQhdr_User_Agent[] = "User-Agent: ";
static char SShdr_Cache_Control_Max_Age[] = "Cache-Control: max-age";
static char SShdr_Cache_Control_No_Cache[] = "Cache-Control: no-cache";
static char SShdr_Cache_Control_No_Store[] = "Cache-Control: no-store";
static char SShdr_Connection_close[] = "Connection: close";
static char SShdr_Connection_Keep_Alive[] = "Connection: Keep-Alive";
static char SShdr_Content_Length[] = "Content-Length: ";
static char SShdr_Date[] = "Date: ";
static char SShdr_ETag[] = "ETag: ";
static char SShdr_ETagW[] = "ETag: W/";
static char SShdr_Expires[] = "Expires: ";
static char SShdr_Keep_Alive[] = "Keep-Alive: ";
static char SShdr_Last_Modified[] = "Last-Modified: ";
static char SShdr_Server[] = "Server: ";
static char SShdr_Set_Cookie[] = "Set-Cookie: ";
static char SShdr_Chunked[] = "Transfer-Encoding: chunked";

enum tokid_e {
	_Hdr_First_,

	Qhdr_Accept_Charset,
	Qhdr_Accept_Encoding,
	Qhdr_Accept_Language,
	Qhdr_Accept,
	Qhdr_Authorization,
	Qhdr_Connection_close,
	Qhdr_Connection_Keep_Alive,
	Qhdr_Date,
	Qhdr_ETag,
	Qhdr_ETagW,
	Qhdr_Host,
	Qhdr_If_Modified_Since,
	Qhdr_If_Unmodified_Since,
	Qhdr_Keep_Alive,
	Qhdr_Pragma_no_cache,
	Qhdr_User_Agent,
	Shdr_Cache_Control_Max_Age,
	Shdr_Cache_Control_No_Cache,
	Shdr_Cache_Control_No_Store,
	Shdr_Connection_close,
	Shdr_Connection_Keep_Alive,
	Shdr_Content_Length,
	Shdr_Date,
	Shdr_ETag,
	Shdr_ETagW,
	Shdr_Expires,
	Shdr_Keep_Alive,
	Shdr_Last_Modified,
	Shdr_Server,
	Shdr_Set_Cookie,
	Shdr_Chunked,
	_Hdr_Last_
};

token_t tokreq[] = {

	INIT(Qhdr_Accept_Charset, PASS | QUALIFIER),
	INIT(Qhdr_Accept_Encoding, PASS | QUALIFIER ),
	INIT(Qhdr_Accept_Language, PASS | QUALIFIER),
	INIT(Qhdr_Accept, PASS | QUALIFIER),
	INIT(Qhdr_Authorization, PASS | QUALIFIER | NOCACHE),
	INIT(Qhdr_Connection_close, QUALIFIER),
	INIT(Qhdr_Connection_Keep_Alive, QUALIFIER),
	INIT(Qhdr_Date, PASS | QUALIFIER | DATE),
	INIT(Qhdr_ETag, PASS | QUALIFIER),
	INIT(Qhdr_ETagW, PASS | NOCACHE),
	INIT(Qhdr_Host, PASS | QUALIFIER | HASH),
	INIT(Qhdr_If_Modified_Since, FILTER | QUALIFIER | DATE),
	INIT(Qhdr_If_Unmodified_Since, FILTER | QUALIFIER | DATE),
	INIT(Qhdr_Keep_Alive, FILTER | QUALIFIER),
	INIT(Qhdr_Pragma_no_cache, PASS | NOCACHE),
	INIT(Qhdr_User_Agent, PASS | QUALIFIER),
	{NULL}
};

#define	tokreq_cnt (sizeof (tokreq) / sizeof (*tokreq))

token_t tokres[] = {

	INIT(Shdr_Cache_Control_Max_Age, PASS | NUMERIC),
	INIT(Shdr_Cache_Control_No_Cache, PASS | NOCACHE),
	INIT(Shdr_Cache_Control_No_Store, PASS | NOCACHE),
	INIT(Shdr_Connection_close, FILTER | QUALIFIER),
	INIT(Shdr_Connection_Keep_Alive, FILTER | QUALIFIER),
	INIT(Shdr_Content_Length, PASS | QUALIFIER | NUMERIC),
	INIT(Shdr_Date, PASS | QUALIFIER | DATE),
	INIT(Shdr_ETag, PASS | QUALIFIER),
	INIT(Shdr_ETagW, PASS | NOCACHE),
	INIT(Shdr_Expires, PASS | QUALIFIER | DATE),
	INIT(Shdr_Keep_Alive, FILTER | QUALIFIER),
	INIT(Shdr_Last_Modified, PASS | QUALIFIER | QUALIFIER | DATE),
	INIT(Shdr_Server, PASS | QUALIFIER),
	INIT(Shdr_Set_Cookie, PASS | NOCACHE),
	INIT(Shdr_Chunked, PASS | QUALIFIER),
	{NULL}
};

#define	tokres_cnt (sizeof (tokres) / sizeof (*tokres))


