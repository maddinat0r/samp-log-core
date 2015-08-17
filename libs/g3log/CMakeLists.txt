# g3log is a KjellKod Logger
# 2011 @author Kjell Hedström, hedstrom@kjellkod.cc 
# ==================================================================
# 2010 by KjellKod.cc. This is PUBLIC DOMAIN to use at your own
#    risk and comes  with no warranties.
#
# This code is yours to share, use and modify with no strings attached
#   and no restrictions or obligations.
# ===================================================================

cmake_minimum_required (VERSION 2.8)
ENABLE_LANGUAGE(CXX)
set(CMAKE_BUILD_TYPE Release)

project (g3log)

SET(64_BIT_OS FALSE)

INCLUDE (${g3log_SOURCE_DIR}/Build.cmake)




# ============================================================================
# G3LOG OPTIONAL FEATURES
# ============================================================================
# Dynamic logging:  ENABLE WITH:     -DUSE_DYNAMIC_LOGGING_LEVELS=ON   : run-type turn on/off levels
# DBUG instead of DEBUG logging level: ENABLE WITH: -DCHANGE_G3LOG_DEBUG_TO_DBUG=ON
#INCLUDE (${g3log_SOURCE_DIR}/Options.cmake)
