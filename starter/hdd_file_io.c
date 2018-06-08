////////////////////////////////////////////////////////////////////////////////
//
//  File           : hdd_file_io.h
//  Description    : This is the implementation of the standardized IO functions
//                   for used to access the HDD storage system.
//
//  Author         : Patrick McDaniel
//  Last Modified  : Sat Sep 2nd 08:56:10 EDT 2017
//
////////////////////////////////////////////////////////////////////////////////
//
// STUDENTS MUST ADD COMMENTS BELOW and FILL INT THE CODE FUNCTIONS
//

// Includes
#include <malloc.h>
#include <string.h>

// Project Includes
#include <hdd_file_io.h>
#include <hdd_driver.h>
#include <hdd_driver.h>
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>


// Defines (you can ignore these)
#define MAX_HDD_FILEDESCR 1024
#define HIO_UNIT_TEST_MAX_WRITE_SIZE 1024
#define HDD_IO_UNIT_TEST_ITERATIONS 10240


// Type for UNIT test interface (you can ignore this)
typedef enum {
	HIO_UNIT_TEST_READ   = 0,
	HIO_UNIT_TEST_WRITE  = 1,
	HIO_UNIT_TEST_APPEND = 2,
	HIO_UNIT_TEST_SEEK   = 3,
} HDD_UNIT_TEST_TYPE;

struct HDD_FILE{
	int16_t fh;		//file handle
	uint32_t blockID;	//blockID
	int init;	//intialize
	uint32_t cp; // current position
};
struct HDD_FILE file;

//
// Implementation

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_open(char*)
// Description  : hdd_open will open a file and return an integer file handle.
//		  and it return -1 on failure and integer on sucess.
//
// Inputs       : path	- the given filename
// Outputs      : fileHandle	- the unique integer that needed to be returned.
//
int16_t hdd_open(char *path) {
	memset(&file.blockID, 0, sizeof(int16_t));	//memory set the 64bits command 
	HddBitCmd command;
	memset(&command, 0, sizeof(int64_t));
	HddBitResp response;
	memset(&response, 0, sizeof(int64_t));		//memory set the 64bits response
	command = 0;		//set command default to 0
	if (strcmp(path, "") == 0){		//case that no file name
		return -1;
	}
	if (file.init == 0){		//initialize the hdd driver
		if (hdd_initialize() == -1){		//reject if not initialized
			printf("HDD driver is not initialized\n");
			return -1;
		}
		file.init = 1;
	}
	file.fh = 0;		//set a file handle
	printf("Open Sucess\n");		//for debug
	return file.fh;

}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_close(int16_t)
// Description  : close the file reference by the file handle
//
// Inputs       : fh	-file handle
// Outputs      : -0 sucess	--1 failure
//

int16_t hdd_close(int16_t fh) {
	if (fh != file.fh){		//case the file handle is incorrect
		printf("file is not correctly opened\n");
		return -1;
	}
	else{
		hdd_delete_block(file.blockID);		//delete the block
		file.fh = -1;		//set file handle, current position and blockID
		file.cp = 0;
		file.blockID = 0;
		printf("File Close sucessfully\n");	//for debug
		return 0;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_read(int16_t, void *, int32_t)
// Description  : read a count number and places them into buffer
//
// Inputs       : fh	-file handle	data	-the file content that needed to put in
//		  count	-count number of bytes from the current position
// Outputs      : --1 failure 	-number of bytes read sucess
//
int32_t hdd_read(int16_t fh, void * data, int32_t count) {
	printf("count: %d\n", count);
	uint32_t blockSize, mid;	char *data1;
	int R;
	HddBitCmd command;
	HddBitResp response;
	printf("Enter read\n");		//for debug
	memset(&command, 0, sizeof(HddBitCmd));		//memory set
	memset(&response, 0, sizeof(HddBitResp));	
	blockSize = hdd_read_block_size(file.blockID);	//set the blocksize
	if (fh != file.fh || file.blockID == 0){	//reject the case that file handle is not correct and block is empty
		printf("The file is not correctly opened\n");
		return -1;
	}
	if (file.cp + count > blockSize){
		command = 0x1 << 26;		//shift to 1 (write)
		command = (command | blockSize) << 36;
		command = command | file.blockID;
		data1 = (char *) malloc(blockSize);		//allocate space for data1
		printf("blocksize: %d\n", blockSize);		//for debug
		printf("cp: %d\n", file.cp);
		response = hdd_data_lane(command, data1);	//response
		mid = blockSize - file.cp;
		memcpy(data, &data1[file.cp], mid);		//memory copy the data1 to data
		file.cp = blockSize;	
		free(data1);		//free data1

		return mid;		// return the count
	}
	command = (uint64_t) ((0x1 << 26) | blockSize) << 36;
	command = command | file.blockID;
	data1 = (char *) malloc(blockSize);
	response = hdd_data_lane(command, data1);
	memcpy(data, &data1[file.cp], count);
	file.cp += count;		//adding the current position
	R = (response >> 32) & 0x01;
	if (R == 1)	return -1;	//reject the case that R = 1
	free(data1);		//free data1
	return count;
	
	
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_write(int16_t, void *, int 32_t)
// Description  : write a count number of bytes at the current position, and create new blocks when number of bytes are exceeded.
//
// Inputs       : fh	-file handle	data	- the file content that needed to put in
//		  count	-count number of bytes from the current position
// Outputs      : --1 if failure -number of written read if sucess
//
int32_t hdd_write(int16_t fh, void *data, int32_t count) {
	HddBitCmd command;
	HddBitResp response;
	char *data1;
	printf("Enter write\n");	// for debug
	memset(&command, 0, sizeof(HddBitCmd));			//memory set
	memset(&response, 0, sizeof(HddBitResp));
	if (fh != file.fh){		// reject the case that file handle is incorrect
		printf("file has not been opened\n");
		return -1;
	}
	else if (file.blockID == 0){		//the case that blockID is empty
		command = (command | count) << 36;
		response = hdd_data_lane(command, data);
		file.blockID = (uint64_t) response;	// set to 64 bits
		file.cp = count;
		return count;	//return the count number
	}
	else{
		uint32_t bs = hdd_read_block_size(file.blockID);	//set the block size
		if (file.cp + count <= bs){		// the case that current position add count is less than the block size that have been created
			data1 = (char *) malloc(bs);	// allocate the data
			command = (uint64_t)((0x1 << 26) | bs) << 36;
			command = command | file.blockID;
			response = hdd_data_lane(command, data1);
			command = 0;		//reset to 0
			command = (uint64_t)((0x2 << 26) | bs) << 36;
			command = command | file.blockID;
			memcpy(&data1[file.cp], data, count);	//memcpy the data1 to data
			response = hdd_data_lane(command, data1);
			file.cp += count;
			free(data1);	//free the data1
			printf("Correctly write when the size is correct\n");	// for debug
			return count;
		}
		else{		//almost doing the same thing as the if statement, modify because the blocks are too small for the number of the bytes
			data1 = (char *) malloc(file.cp + count);		
			command = (uint64_t) ((0x1 << 26) | bs)<< 36;
			command = command | file.blockID;
			response = hdd_data_lane(command, data1);
			command = 0;
			command = (command | (file.cp + count)) << 36;
			memcpy(&data1[file.cp], data, count);
			response = hdd_data_lane(command, data1);
			hdd_delete_block(file.blockID);
			file.blockID = (uint64_t) response;
			printf("cp: %d\n", file.cp);
			file.cp += count;
			printf("cp: %d\n", file.cp);
			free(data1);
			printf("Correctly write when the size need to be expanded\n");
			printf("cp: %d\n", file.cp);
			return count;
		}
	}
	return -1;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_seek(int16_t, uint32_t)
// Description  : changes the current seek position of the file
//
// Inputs       : fh	-file handle	loc	-the location that it wants
// Outputs      : --1 on failure and 0 on sucess
//

int32_t hdd_seek(int16_t fh, uint32_t loc) {
	printf("Enter seek\n");		// for debug
	if (fh != file.fh){		// reject the case that file handle is not correct
		printf("The file is not correctly opened, check hdd_seek\n");	
		return -1;
	}
	if(loc > hdd_read_block_size(file.blockID))	return -1;	//reject the case that location exceeds the maximum size of block
	file.cp = loc;
	printf("Seek\n");	// for debug
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hddIOUnitTest
// Description  : Perform a test of the HDD IO implementation
//
// Inputs       : None
// Outputs      : 0 if successful or -1 if failure
//
// STUDENTS DO NOT MODIFY CODE BELOW UNLESS TOLD BY TA AND/OR INSTRUCTOR 
//
int hddIOUnitTest(void) {

	// Local variables
	uint8_t ch;
	int16_t fh, i;
	int32_t cio_utest_length, cio_utest_position, count, bytes, expected;
	char *cio_utest_buffer, *tbuf;
	HDD_UNIT_TEST_TYPE cmd;
	char lstr[1024];

	// Setup some operating buffers, zero out the mirrored file contents
	cio_utest_buffer = malloc(HDD_MAX_BLOCK_SIZE);
	tbuf = malloc(HDD_MAX_BLOCK_SIZE);
	memset(cio_utest_buffer, 0x0, HDD_MAX_BLOCK_SIZE);
	cio_utest_length = 0;
	cio_utest_position = 0;

	// Start by opening a file
	fh = hdd_open("temp_file.txt");
	if (fh == -1) {
		logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : Failure open operation.");
		return(-1);
	}

	// Now do a bunch of operations
	for (i=0; i<HDD_IO_UNIT_TEST_ITERATIONS; i++) {

		// Pick a random command
		if (cio_utest_length == 0) {
			cmd = HIO_UNIT_TEST_WRITE;
		} else {
			cmd = getRandomValue(HIO_UNIT_TEST_READ, HIO_UNIT_TEST_SEEK);
		}

		// Execute the command
		switch (cmd) {

		case HIO_UNIT_TEST_READ: // read a random set of data
			count = getRandomValue(0, cio_utest_length);
			logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : read %d at position %d", bytes, cio_utest_position);
			bytes = hdd_read(fh, tbuf, count);
			if (bytes == -1) {
				logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : Read failure.");
				return(-1);
			}

			// Compare to what we expected
			if (cio_utest_position+count > cio_utest_length) {
				expected = cio_utest_length-cio_utest_position;
			} else {
				expected = count;
			}
			if (bytes != expected) {
				logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : short/long read of [%d!=%d]", bytes, expected);
				return(-1);
			}
			if ( (bytes > 0) && (memcmp(&cio_utest_buffer[cio_utest_position], tbuf, bytes)) ) {

				bufToString((unsigned char *)tbuf, bytes, (unsigned char *)lstr, 1024 );
				logMessage(LOG_INFO_LEVEL, "HIO_UTEST R: %s", lstr);
				bufToString((unsigned char *)&cio_utest_buffer[cio_utest_position], bytes, (unsigned char *)lstr, 1024 );
				logMessage(LOG_INFO_LEVEL, "HIO_UTEST U: %s", lstr);

				logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : read data mismatch (%d)", bytes);
				return(-1);
			}
			logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : read %d match", bytes);


			// update the position pointer
			cio_utest_position += bytes;
			break;

		case HIO_UNIT_TEST_APPEND: // Append data onto the end of the file
			// Create random block, check to make sure that the write is not too large
			ch = getRandomValue(0, 0xff);
			count =  getRandomValue(1, HIO_UNIT_TEST_MAX_WRITE_SIZE);
			if (cio_utest_length+count >= HDD_MAX_BLOCK_SIZE) {

				// Log, seek to end of file, create random value
				logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : append of %d bytes [%x]", count, ch);
				logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : seek to position %d", cio_utest_length);
				if (hdd_seek(fh, cio_utest_length)) {
					logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : seek failed [%d].", cio_utest_length);
					return(-1);
				}
				cio_utest_position = cio_utest_length;
				memset(&cio_utest_buffer[cio_utest_position], ch, count);

				// Now write
				bytes = hdd_write(fh, &cio_utest_buffer[cio_utest_position], count);
				if (bytes != count) {
					logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : append failed [%d].", count);
					return(-1);
				}
				cio_utest_length = cio_utest_position += bytes;
			}
			break;

		case HIO_UNIT_TEST_WRITE: // Write random block to the file
			ch = getRandomValue(0, 0xff);
			count =  getRandomValue(1, HIO_UNIT_TEST_MAX_WRITE_SIZE);
			// Check to make sure that the write is not too large
			if (cio_utest_length+count < HDD_MAX_BLOCK_SIZE) {
				// Log the write, perform it
				logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : write of %d bytes [%x]", count, ch);
				memset(&cio_utest_buffer[cio_utest_position], ch, count);
				bytes = hdd_write(fh, &cio_utest_buffer[cio_utest_position], count);
				if (bytes!=count) {
					logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : write failed [%d].", count);
					return(-1);
				}
				cio_utest_position += bytes;
				if (cio_utest_position > cio_utest_length) {
					cio_utest_length = cio_utest_position;
				}
			}
			break;

		case HIO_UNIT_TEST_SEEK:
			count = getRandomValue(0, cio_utest_length);
			logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : seek to position %d", count);
			if (hdd_seek(fh, count)) {
				logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : seek failed [%d].", count);
				return(-1);
			}
			cio_utest_position = count;
			break;

		default: // This should never happen
			CMPSC_ASSERT0(0, "HDD_IO_UNIT_TEST : illegal test command.");
			break;

		}
	}

	// Close the files and cleanup buffers, assert on failure
	if (hdd_close(fh)) {
		logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : Failure read comparison block.", fh);
		return(-1);
	}
	free(cio_utest_buffer);
	free(tbuf);

	// Return successfully
	return(0);
}

































