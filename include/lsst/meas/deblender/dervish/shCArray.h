#ifndef	SHCARRAY_H
#define	SHCARRAY_H

/*++
 * FACILITY:	Dervish
 *
 * ABSTRACT:	General arrays for Dervish.
 *
 * ENVIRONMENT:	ANSI C.
 *		shCArray.h
 *
 * AUTHOR:	Tom Nicinski, Creation date: 14-Feb-1994
 *--
 ******************************************************************************/

#include	"shCSchema.h"

/******************************************************************************/

/******************************************************************************/
/*                                                                            *
 * In order to make this header file somewhat compatible with the current     *
 * object schema generation (with makeio), the following restrictions are     *
 * adhered to:                                                                *
 *                                                                            *
 *   o   Object schemas are flagged by using typedefs.                        *
 *                                                                            *
 *          o   "typedef" must be in the first column                         *
 *                                                                            *
 *          o   "typedef" must be followed by 1 blank and "enum" or "struct". *
 *                                                                            *
 *          o   structs cannot be defined inside the object schema. They must *
 *              be defined as object schemas also.                            *
 *                                                                            *
 *          o   If "pragma" information is included, it must be contained in  *
 *              a comment that starts on the same line as the typedef name.   *
 *                                                                            *
 *          o   Arrays must specify their size as an explicit number or a     *
 *		simple macro -- no arithmetic can be used.                    *
 *                                                                            *
 *          o   No unusual types can be used to declare elements within an    *
 *              object schema.                                                *
 *                                                                            *
 ******************************************************************************/

#include	"dervish_msg_c.h"		/* For RET_CODE                       */

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************/
/*
 * Data is accessed through the arrayPtr and data fields of the ARRAY structure.
 * These two fields describe arbitrary n-dimensional arrays and allow access to
 * them.
 *
 * Notation used throughout follows C's row-major ordering of array elements,
 * that is, the last subscript varies most rapidly.  The analogy to C arrays
 * applies only to notation (other limitations with respect to the notation
 * also apply).  Since the ARRAY format is generic where array dimensions are
 * not known until run-time, arrays are stored in such a manner that regular C
 * syntax can be used to access these arrays without knowing their dimensions
 * at compilation-time (thus, they're not stored the same a C arrays).  The C
 * concept of an array name representing a pointer to the array is used to
 * accomplish this type of addressing.  For n-dimensional arrays, n sets of
 * pointers are used.  The first pointer is associated with the array name. It
 * points to the data in a 1-dimensional array or to the second set of pointers.
 * These pointers in turn point to the data in a 2-dimensional array or to the
 * third set of pointers; and so forth.
 *
 * Describing Scalars and Arrays by Example
 * ----------------------------------------
 *
 * At a minimum, valid ARRAYs must be 1-dimensional.  The n-dimensional examples
 * in subsequent sections show the user declaring myArray in order to reference
 * array data, but never setting its value (pointing it off to data).  Consider
 * the following example, in C, where array points to a ARRAY structure:
 *
 *     #include "shCArray.h"
 *
 *     short int   **myArray;       \* We know a priori this is a 2-D array   *\
 *               .                  \* of shorts.                             *\
 *               :
 *     myArray = ((short int **)array->arrayPtr);
 *               .
 *               :
 *     ... myArray[y][x] ...
 *
 * array->arrayPtr should be used to initialize myArray in all cases.  array->
 * data, the pointer to the first byte of data in the array, should not be used.
 * If C's array referencing notation is to be used, array->data would only work
 * properly if the data was 1-dimensional.  array->arrayPtr will work for any
 * n-dimensional array.
 *
 * 1-Dimensional Arrays
 * --------------------
 *
 * A 1-dimensional array of i elements is stored as follows:
 *
 *          +-------+     +-----+
 * myArray: |   o---|---->| [0] |
 *          +-------+     +-----+
 *                        | [1] |
 *                        +-----+
 *                        /  .  /
 *                        /  :  /
 *                        +-----+
 *                        |[i-1]|
 *                        +-----+
 *
 * where the C declaration
 *
 *      int     *myArray;	\* Ptr to (array of) int                      *\
 *
 * will allow element x to be accessed with a C expression such as
 *
 *      ... myArray[x] ...
 *
 * 2-Dimensional Arrays
 * --------------------
 *
 * A 2-dimensional array of j by i elements is stored as follows:
 *
 *          +-------+     +-------+     +-----+-----+--//--+-----+
 * myArray: |   o---|---->|[0]  o-|---->| [0] | [1] |  ... |[i-1]|
 *          +-------+     +-------+     +-----+-----+--//--+-----+
 *                        |[1]  o-|--.
 *                        +-------+  |  +-----+-----+--//--+-----+
 *                        |   .   |  `->| [0] | [1] |  ... |[i-1]|
 *                        /   :   /     +-----+-----+--//--+-----+
 *                        |       |
 *                        +-------+     +-----+-----+--//--+-----+
 *                        |[j-1]o-|---->| [0] | [1] |  ... |[i-1]|
 *                        +-------+     +-----+-----+--//--+-----+
 *
 * where the C declaration
 *
 *      int    **myArray;	\* Ptr to ptr to (array of) int               *\
 *
 * will allow element [y,x] to be accessed with a C expression such as
 *
 *      ... myArray[y][x] ...
 *
 * If myArray were declared as char **, the above example will access one
 * character from a character string.  The whole string is referenced with
 *
 *      ... myArray[y] ...
 *
 * 3-Dimensional Arrays
 * --------------------
 *
 * A 3-dimensional array of k by j by i elements is stored as follows:
 *
 *                                       +-----+ +-----+        +-----+
 *                                    .->| [0] | | [0] |<-.  .->| [0] |
 *                                    |  +-----+ +-----+  |  |  +-----+
 *                                    |  | [1] | | [1] |  |  |  | [1] |
 *                                    |  +-----+ +-----+  |  |  +-----+
 *                                    |  /  .  / /  .  /  |  |  /  .  /
 *                                    |  /  :  / /  :  /  |  |  /  :  /
 *                                    |  +-----+ +-----+  |  |  +-----+
 *                                    |  |[i-1]| |[i-1]|  |  |  |[i-1]|
 *                                    |  +-----+ +-----+  |  |  +-----+
 *          +-------+     +-------+   `-------.       .---'  `-------.
 * myArray: |   o---|---->|[0]  o-|---        |       |              |
 *          +-------+     +-------+     +-----|-+-----|-+--//--+-----|-+
 *                        |[1]  o-|---->|[0]  o |[1]  o |  ... |[j-1]o |
 *                        +-------+     +-------+-------+--//--+-------+
 *                        |   .   |
 *                        /   :   /     +-------+-------+--//--+-------+
 *                        |       |  .->|[0]  o |[1]  o |  ... |[j-1]o |
 *                        +-------+  |  +-----|-+-----|-+--//--+-----|-+
 *                        |[k-1]o-|--'        |       |              |
 *                        +-------+           V       V              V
 *                                       +-----+ +-----+        +-----+
 *                                       | [0] | | [0] |        | [0] |
 *                                       +-----+ +-----+        +-----+
 *                                       | [1] | | [1] |        | [1] |
 *                                       +-----+ +-----+        +-----+
 *                                       /  .  / /  .  /        /  .  /
 *                                       /  :  / /  :  /        /  :  /
 *                                       +-----+ +-----+        +-----+
 *                                       |[i-1]| |[i-1]|        |[i-1]|
 *                                       +-----+ +-----+        +-----+
 *
 * where the C declaration
 *
 *     int   ***myArray;	\* Ptr to ptr to ptr to (array of) int        *\
 *
 * will allow element [z,y,x] to be accessed with a C expression such as
 *
 *      ... myArray[z][y][x] ...
 *
 * 4-Dimensional Arrays
 * --------------------
 *
 * A 4-dimensional array of m by k by j by i elements is stored as follows:
 *
 *  +-----+--//--+-----+                                    +-----+--//--+-----+
 *  | [0] |  ... |[i-1]|<-.  +-------+        +-------+  .->| [0] |  ... |[i-1]|
 *  +-----+--//--+-----+   `-|-o  [0]|<-.  .->|[0]  o-|-'   +-----+--//--+-----+
 *                           +-------+  |  |  +-------+
 *  +-----+--//--+-----+   .-|-o  [1]|  |  |  |[1]  o-|-.   +-----+--//--+-----+
 *  | [0] |  ... |[i-1]|<-'  +-------+  |  |  +-------+  `->| [0] |  ... |[i-1]|
 *  +-----+--//--+-----+     /   .   /  |  |  /   .   /     +-----+--//--+-----+
 *                           /   :   /  |  |  /   :   /
 *  +-----+--//--+-----+     +-------+  |  |  +-------+     +-----+--//--+-----+
 *  | [0] |  ... |[i-1]|<----|-o[j-1]|  |  |  |[j-1]o-|---->| [0] |  ... |[i-1]|
 *  +-----+--//--+-----+     +-------+  |  |  +-------+     +-----+--//--+-----+
 *                                      |  `-----------------.
 *          +-------+     +-------+     `-----.              |
 * myArray: |   o---|---->|[0]  o-|---        |              |
 *          +-------+     +-------+     +-----|-+--//--+-----|-+
 *                        |[1]  o-|---->|[0]  o |  ... |[k-1]o |
 *                        +-------+     +-------+--//--+-------+
 *                        |   .   |
 *                        /   :   /     +-------+--//--+-------+
 *                        |       |  .->|[0]  o |  ... |[k-1]o |
 *                        +-------+  |  +-----|-+--//--+-----|-+
 *                        |[m-1]o-|--'        |              |
 *                        +-------+     .-----'              |
 *                                      |  .-----------------'
 *  +-----+--//--+-----+                |  |                +-----+--//--+-----+
 *  | [0] |  ... |[i-1]|<-.  +-------+  |  |  +-------+  .->| [0] |  ... |[i-1]|
 *  +-----+--//--+-----+   `-|-o  [0]|<-'  `->|[0]  o-|-'   +-----+--//--+-----+
 *                           +-------+        +-------+
 *  +-----+--//--+-----+   .-|-o  [1]|        |[1]  o-|-.   +-----+--//--+-----+
 *  | [0] |  ... |[i-1]|<-'  +-------+        +-------+  `->| [0] |  ... |[i-1]|
 *  +-----+--//--+-----+     /   .   /        /   .   /     +-----+--//--+-----+
 *                           /   :   /        /   :   /
 *  +-----+--//--+-----+     +-------+        +-------+     +-----+--//--+-----+
 *  | [0] |  ... |[i-1]|<----|-o[j-1]|        |[j-1]o-|---->| [0] |  ... |[i-1]|
 *  +-----+--//--+-----+     +-------+        +-------+     +-----+--//--+-----+
 *
 * where the C declaration
 *
 *      int  ****myArray;	\* Ptr to ptr to ptr to ptr to (array of) int *\
 *
 * will allow element [t,z,y,x] to be accessed with a C expression such as
 *
 *      ... myArray[t][z][y][x] ...
 *
 * Limitations in Using the C Array Notation
 * -----------------------------------------
 *
 * As mentioned above, the C notation for accessing arrays used here only
 * reflects the access of data, not the layout of data in memory.  This nota-
 * tion has some other restrictions that apply:
 *
 *   o	Declaring arrays as pointers to pointers ... to pointers to (an array)
 *	of a type does not permit any array bounds checking.
 *
 *   o	The C array reference notation can only be used to access a single
 *	element with all indices, or to access the innermost array (the one
 *	whose index changes the fastest).  Consider:
 *
 *	     int   ****myArray;             \* 4-D array of size [m][k][j][i] *\
 *
 *	     ... myArray[t][z][y][x] ...    \* Access one element             *\
 *	     ... myArray[t][z][y]    ...    \* Access an array (1-dimensional)*\
 *	     ... myArray[t][z]       ...    \* Access an array of pointers    *\
 *
 *	The first two array references behave as expected.  But, the last array
 *	reference will not properly provide the address of a 2-dimensional
 *	array, as expected in regular C code.  Instead, it will return the
 *	address of an array of pointers.
 *
 *   o	Structures are a bit more difficult to reference easily with the C array
 *	notation.  These data types consist of two or more datums for each
 *	element in the array.  Consider a pair of IEEE 32-bit precision floating
 *	point numbers that make up a complex complex value.  To access such a
 *	value, in a 4-dimensional array, the C declaration would be
 *
 *	     typedef struct
 *	        {
 *	        float           realPart;   \* Real      portion of complex # *\
 *	        float           imagPart;   \* Imaginary portion of complex # *\
 *	        } COMPLEXVAL;
 *
 *	     COMPLEXVAL ****myArray;        \* 4-D array of size [m][k][j][i] *\
 *
 *	This style of declaration will allow element [t,z,y,x] to be accessed
 *	with a C expression such as
 *
 *	     ... myArray[t][z][y][x].realPart ...
 *
 *	Is this   r e a l l y   true?  This needs to be tested.
 *	
 * Limiting the Number of Dimensions
 * ---------------------------------
 *
 * The maximum number of ARRAY dimensions is limited to 34.  This value should
 * be large enough for most applications.  The value was chosen based on the
 * practical number of dimensions that a FITS Binary Table could support.
 * Because of the FITS header line size (80 bytes) and the format of the TDIMn
 * keyword, FITS is practically limited to 33 dimensions.  The additional dim-
 * ension permits the slowest varying (first) index to reference the Table row.
 *
 * Physical Layout of Array Data in Memory
 * ---------------------------------------
 *
 * Now that the limitations in using the C array notation have been described,
 * they can be relaxed a bit.  Data is stored in a C fashion (row-major) with
 * the fastest varying indexed elements being stored adjacent to each other,
 * then the next fastest varying indexed array is stored, etc.  Consider an
 * example of a 3-dimensional array of k by j by i elements (addresses increase
 * left to right and top to bottom):
 *
 *   k        .-- j-index [0] ---.-- j-index [1] ---.      .-- j-index [j-1] -.
 * index       |                  |                  |      |                  |
 *   |         V                  V                  V      V                  V
 *   V   .-->  +-----+--//--+-----+-----+--//--+-----+--//--+-----+--//--+-----+
 *  [0]  |     | [0] |  ... |[i-1]| [0] |  ... |[i-1]|  ... | [0] |  ... |[i-1]|
 *       +-->  +-----+--//--+-----+-----+--//--+-----+--//--+-----+--//--+-----+
 *  [1]  |     | [0] |  ... |[i-1]| [0] |  ... |[i-1]|  ... | [0] |  ... |[i-1]|
 *       +-->  +-----+--//--+-----+-----+--//--+-----+--//--+-----+--//--+-----+
 *   .         /                                  .                            /
 *   :         /                                  :                            /
 *       +-->  +-----+--//--+-----+-----+--//--+-----+--//--+-----+--//--+-----+
 * [k-1] |     | [0] |  ... |[i-1]| [0] |  ... |[i-1]|  ... | [0] |  ... |[i-1]|
 *       `-->  +-----+--//--+-----+-----+--//--+-----+--//--+-----+--//--+-----+
 *
 * If the 3-dimensional array size is known at compilation time, for example
 * consider a function argument formally declared as
 *
 *      int  myArray[][j][i];	\* 3-dimensional array w/ unknown dimension   *\
 *
 * then the normal C array referencing notation can also be used
 *
 *      ... myArray[z][y][x] ...
 *
 * Notice that the slowest varying dimension does not need to be known at compi-
 * lation (as the slowest varying index can be left unspecified in C).  If the
 * source was compiled with array bounds checking, the j and i bounds will be
 * checked, but the unknown dimension index will not be checked.
 *
 * Computing Array Element Locations
 * ---------------------------------
 *
 * Rather than bouncing down the hierarchy of array pointers, it is possible to
 * compute the location of an array element, given its indices (0-indexed for
 * this description). ARRAY's subArrCnt contains the count of array elements at
 * each "level" within the array.  There are dimCnt valid elements in subArrCnt
 * (just as for dim).

 * The level (0-indexed) refers to the depth within the hierarchy of array
 * pointers, where level n-1 contains the leaves, namely the array data itself.
 * The level also refers to the number of array indices provided, less 1.
 * Consider a 3-dimensional array with dimensions (2, 3, 2) (in row-major
 * order):
 *
 *      level
 *      -----
 *       -1            ___.___
 *                    /       \
 *                   /         \
 *        0        _0_         _1_  <------+-- slowest varying indices
 *                /   \       /   \        |
 *               /     \     /     \       |   hierarchy of array
 *        1     0---1---2   0---1---2      |   pointers (arrayPtr)
 *             /_\ /_\ /_\ /_\ /_\ /_\     |
 *             0 1 0 1 0 1 0 1 0 1 0 1  <--+-- fastest varying indices
 *            +-+-+-+-+-+-+-+-+-+-+-+-+  --.
 *        2   | | | | | | | | | | | | |    |   data (data.dataPtr)
 *            +-+-+-+-+-+-+-+-+-+-+-+-+  --'
 *
 * At level l (0 <= l < dimCnt), there are dim[i] subarrays.  In the above
 * example, at level 0, there are two subarrays.  If one index is provided, it
 * explicitly refers to only one subarray at level 0 (as the index references
 * only the slowest varying index).  Passing two indices references only one of
 * six subarrays at level 1.  Passing three indices references only one of 12
 * array elements at level 2.  Level -1 does not exist in ARRAY's subArrCnt.
 * But, providing no indices to reference an entire array is equivalent to
 * accessing a subarray at level -1.
 *
 * For an n-dimensional array with dimensions (in row-major order)
 *
 *      (d , d , ..., d   )
 *        0   1        n-1
 *
 * each subarray size as an array element count, a, at level l (0 <= l < n-1) is
 *
 *           ____n-1
 *            ||
 *      a  =  ||    d
 *       l    ||     j
 *             j=l+1
 * 
 * By definition,
 *
 *      a  =  1
 *       n-1
 *
 * Because array elements are layed out sequentially, the array may be treated
 * 1-dimensionally.  Given a set of n indices, x, into the n-dimensional array
 *
 *      (x , x , ..., x   )
 *        0   1        n-1
 *
 * a 1-dimensional index, i, can be computed
 *
 *                 n-2
 *             .---.
 *       x   +  \     a * x
 *        n-1   /      k   k
 *             '---'
 *                 k=0
 *
 * If fewer than n indices were passed for x, the remaining indices should be
 * treated as zeros (as it's 0-indexed) to locate the starting array element of 
 * a subarray.  The above expression degenerates by changing the upper bound on
 * the sum to the number of passed indices, j, less 1 and dropping the initial
 * addend:
 *
 *                 j-1
 *             .---.
 *              \     a * x
 *              /      k   k
 *             '---'
 *                 k=0
 *
 * In either case, multiplying i by data.incr results with the array offset
 * (from data.dataPtr) for the element or subarray (respectively) indexed by x.
 *
 * These computations can be done using subArrCnt.  For example, the following
 * C code returns elemOff, the 0-indexed offset (expressed in array element
 * units) to the start of the referenced subarray.  idxCnt is the number of
 * indices (idx) passed by the caller (which must be less than the number of
 * array dimensions) and array points to the desired ARRAY:
 *
 *      if (idxCnt == array->dimCnt) { elemOff = idx[idxCnt-1]; dimIdx = idxCnt - 2; }
 *                             else  { elemOff =            0;  dimIdx = idxCnt - 1; }
 *      for ( ; dimIdx >= 0; dimIdx--)
 *         {
 *         elemOff += (array->subArrCnt[dimIdx] * idx[dimIdx]);
 *         }
 *
 * The number of elements within a subarray, subArrSize, referenced by idxCnt
 * indices can be gotten with:
 *
 *      subArrSize = (idxCnt > 0) ? array->subArrCnt[idxCnt-1]
 *                                : array->subArrCnt[0] * array->dim[0];
 *
 * Specific handling for the case where no indices are passed is needed, as
 * level -1 does not exist.
 *
 * Given a 0-indexed 1-dimensional offset, elemOff, into an n-dimensional array,
 * the corresponding n indices can be computed.  Using subArrCnt, the following
 * C code illustrates how this can be accomplished:
 *
 *      elemIdx = elemOff;
 *      for (dimIdx = 0; dimIdx < (array->dimCnt - 1); dimIdx++)
 *         {
 *         indices[dimIdx] = elemIdx /  array->subArrCnt[dimIdx];
 *                           elemIdx %= array->subArrCnt[dimIdx];
 *         }
 *         indices[dimIdx] = elemIdx;      \* Use final remainder       *\
 *
 * All variables are signed integers.
 *
 * As mentioned before, STR object schema types are handled somewhat unusually.
 * For STRs, the subarray count represents the total number of characters
 * (including null terminators) in the subarray rather than the number of
 * character strings.
 *
 ******************************************************************************/

/******************************************************************************/
/*
 *   dimCnt	The number of dimensions.
 *
 *   dim	The dimensions of the array.  The dimensions are in row-major
 *		order, where the fastest changing index is on the right.  This
 *		field is fixed in size.
 *
 *		NOTE:	At a minimum, this field should be able to support Dervish
 *			applications.  For example, it should be at least 34 to
 *			support FITS Binary Tables: 1 dimension for a Table row
 *			index and 33 dimensions for the practical maximum number
 *			of dimensions a Binary Table can handle (although the
 *			FITS standard doesn't restrict the number of dimensions,
 *			the size limitations of a FITS header line and the TDIMn
 *			keyword format practically restrict dimension count).
 *
 *   subArrCnt	The number of elements in a subarray based at each "level"
 *		within the array.  At level l, there are dim[l] subarrays.
 *
 *   arrayPtr	A hierarchy of pointers (directed acyclic tree) permits arrays
 *		to be accessed with C array referencing notation without knowing
 *		array dimensions at compilation.
 *
 *   data.dataPtr	Pointer to first byte of data.
 *
 *   data.schemaType	The object schema type of the data.
 *
 *   data.size		Size of the object schema in bytes.
 *
 *   data.align		Alignment factor for first object schema.
 *
 *   data.incr		Address increment between object schemas.
 *
 *   nStar	The amount of indirection OUTSIDE of the ARRAY.  A zero value
 *		indicates that all array data is maintained by the ARRAY struc-
 *		ture (in its data area).  A non-zero value indicates that data
 *		exists outside the maintenance area of ARRAY.  In essence, this
 *		is a count of the number of stars (asterisks) that would be used
 *		in a C declaration.
 *
 *   info	A pointer to an optional (auxiliary) structure describing the
 *		array data further.
 *
 *   infoType	The object schema type of the auxiliary information.  If an
 *		object schema is not defined for the information, it should
 *		be of type "UNKNOWN".
 */

typedef struct	 shArray_Data
   {
   unsigned       char	*dataPtr;	/* 1st datum (dataPtr[0, 0, ..., 0])  */
   TYPE			 schemaType;
   unsigned long  int	 size;		/* Size of object schema (bytes)      */
   unsigned       int	 align;		/* Alignment factor                   */
   unsigned       int	 incr;		/* Address increment to next object   */
   }		   ARRAY_DATA;		/*
					   pragma   SCHEMA
					 */


#define		 shArrayDimLim	     34

typedef struct	 shArray_Array
   {
                  int	 dimCnt;	/* Number of dimensions               */
                  int	 dim    [shArrayDimLim]; /* Dimensions                */
	    long  int  subArrCnt[shArrayDimLim]; /* Subarray element counts   */
   void			*arrayPtr;	/* Array pointer hierarchy root       */
   ARRAY_DATA		 data;		/* 1st datum (data[0,0,...,0])        */
                  int	 nStar;		/* Amount of indirection outside ARRAY*/
   void			*info;		/* Additional data-specific info      */
   TYPE			 infoType;	/* Additional info structure type     */
   }		   ARRAY;		/*
					   pragma   CONSTRUCTOR
					 */

/******************************************************************************/

/******************************************************************************/
/*
 * API for FITS Tables.
 *
 * Public  Function		Explanation
 * ----------------------------	------------------------------------------------
 *   shArrayDataAlloc		Allocate space for array data and pointers.
 *   shArrayDataFree		Free     space for array data and pointers.
 *
 * Private Function		Explanation
 * ----------------------------	------------------------------------------------
 * p_shArrayLink		Link up a multidimension array of ptrs to data.
 *   shArrayNew			Object schema constructor for ARRAY.
 *   shArrayDel			Object schema destructor  for ARRAY.
 */

RET_CODE	   shArrayDataAlloc	(ARRAY *array, char *schemaName);
RET_CODE	   shArrayDataFree	(ARRAY *array);

RET_CODE	 p_shArrayLink		(int nDim, int dim[], void *ptrArea, unsigned char *dataArea, unsigned int dataIncr);
ARRAY		*  shArrayNew		(void);
void		   shArrayDel		(ARRAY *array);

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif	/* ifndef SHARRAY_H */
