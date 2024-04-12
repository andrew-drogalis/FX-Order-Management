// Copyright 2024, Andrew Drogalis
// GNU License

#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#ifndef VAR_DECLS
#define _DECL extern
#define _INIT(x)
#else
#define _DECL
#define _INIT(x) = x
#endif

#include <string>

namespace fxordermgmt
{

_DECL std::string const account_username _INIT("BLANK");

_DECL std::string const paper_account_username _INIT("BLANK");

_DECL std::string const forex_api_key _INIT("BLANK");

// Store Passwords in Keyring.

}// namespace fxordermgmt

#endif
