#pragma once

#define ORMPP_ENABLE_LOG
#define ORMPP_ENABLE_MYSQL

#include <ormpp/iguana/util.hpp>

#include <ormpp/type_mapping.hpp>
using blob = ormpp::ormpp_mysql::blob;

#include <ormpp/dbng.hpp>
#include <ormpp/mysql.hpp>
