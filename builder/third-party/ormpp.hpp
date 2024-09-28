#pragma once

// #define ORMPP_ENABLE_LOG
// #define ORMPP_ENABLE_PG
#define ORMPP_ENABLE_MYSQL

#include <ormpp/iguana/util.hpp>

#include <ormpp/type_mapping.hpp>
using blob = ormpp::ormpp_mysql::blob;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <ormpp/dbng.hpp>
#include <ormpp/mysql.hpp>
#pragma GCC diagnostic pop
