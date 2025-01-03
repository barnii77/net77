#ifndef NET77_ERROR_MANAGEMENT_H
#define NET77_ERROR_MANAGEMENT_H

/**
 * int typedef which signifies that the value describes an error where \n
 * -> 0 means no error \n
 * -> x != 0 means error
 */
typedef int ErrorStatus;

/**
 * int typedef which signifies that the value describes a success boolean where \n
 * -> 1 (or x != 0) means success \n
 * -> 0 means failure
 */
typedef int SuccessStatus;

#define errStatusIsOk(err_status) ((err_status) == 0)
#define errStatusIsErr(err_status) ((err_status) != 0)
#define successStatusIsOk(success_status) ((success_status) == 0)
#define successStatusIsErr(success_status) ((success_status) != 0)

#endif