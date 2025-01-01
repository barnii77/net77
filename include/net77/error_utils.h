#ifndef NET77_ERROR_MANAGEMENT_H
#define NET77_ERROR_MANAGEMENT_H

typedef int ErrorStatus;
typedef int SuccessStatus;

#define errStatusIsOk(err_status) ((err_status) == 0)
#define errStatusIsErr(err_status) ((err_status) != 0)
#define successStatusIsOk(success_status) ((success_status) == 0)
#define successStatusIsErr(success_status) ((success_status) != 0)

#endif