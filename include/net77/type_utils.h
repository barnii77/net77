#ifndef NET77_TYPE_UTILS_H
#define NET77_TYPE_UTILS_H

// clang supports these extensions
#ifdef __clang__
/// may be NULL
#define nullable _Nullable
/// must not be NULL
#define nonnull _Nonnull
// other compilers don't support these extensions
#else
/// may be NULL
#define nullable  /* nothing... optional pointer annotation to mark as nullable for informative purposes */
/// must not be NULL
#define nonnull   /* nothing... optional pointer annotation to mark as *not* nullable for informative purposes */
#endif

/// a non-brain-damaged boolean type (cpp style bool)
typedef enum bool {
    false = 0,
    true = 1,
} bool;

/**
 * int typedef which signifies that the value describes an error where \n
 * -> 0 means no error \n
 * -> x != 0 means error
 */
typedef int ErrorStatus;

/**
 * int typedef which signifies that the value describes a success boolean where \n
 * -> true (1, or x != 0) means success \n
 * -> false (0) means failure
 */
typedef bool SuccessStatus;

#endif