#include <err.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>       
#include <sys/stat.h>      
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

void errorHandler(int code, int fd1, int fd2, int fd3, uint32_t* buffer, int errnum)
{
	if (fd1 != -1)
		close(fd1);
	if (fd2 != -1)
		close(fd2);
	if (fd3 != -1)
		close(fd3);

	free(buffer);
	errno = errnum;
	err(code, "error");
}

int cmp(const void* a, const void* b)
{
	if ((*(uint32_t*)a < *(uint32_t*)b))
		return -1;	
	else if ((*(uint32_t*)a == *(uint32_t*)b)) 
		return 0;
	else
		return 1;
}

int main(int argc, char** argv)
{
	if (argc != 2)
		errx(1, "usage: ./main filename");

	if (access(argv[1], F_OK) != 0)
		errx(2, "file does not exist");

	if (access(argv[1], R_OK) != 0)
		errx(3, "file is not readable");

	if (access(argv[1], W_OK) != 0)
		errx(4, "file is not writable");
	
	struct stat st;
	if (stat(argv[1], &st) == -1)
		errorHandler(5, -1, -1, -1, NULL, errno);

	if (st.st_size % sizeof(uint32_t) != 0)
		errx(6, "file does not have only uint32_t numbers");

	uint32_t numEl = st.st_size / sizeof(uint32_t);
	uint32_t lHalf = numEl / 2;
	uint32_t *left = malloc(lHalf * sizeof(uint32_t));

	if (left == NULL)
		errorHandler(7, -1, -1, -1, NULL, errno);

	int readFrom = open(argv[1], O_RDONLY);
	if (readFrom == -1)
		errorHandler(8, readFrom, -1, -1, left, errno);

	int temp1 = open("temp1", O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
	if (temp1 == -1)
		errorHandler(9, temp1, readFrom, -1, left, errno);

	size_t res = read(readFrom, left, lHalf*sizeof(uint32_t));
	if (res != lHalf*sizeof(uint32_t))
		errorHandler(10, temp1, readFrom, -1, left, errno);

	qsort(left, lHalf, sizeof(uint32_t), cmp);
	res = write(temp1, left, lHalf*sizeof(uint32_t));
	if (res != lHalf*sizeof(uint32_t))
		errorHandler(11, temp1, readFrom, -1, left, errno);

	free(left);

	int temp2 = open("temp2", O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
	if (temp2 == -1)
		errorHandler(12, temp1, readFrom, temp2, NULL, errno);

	uint32_t rHalf = numEl - lHalf;
	uint32_t *right = malloc(rHalf * sizeof(uint32_t));
	if (right == NULL)
		errorHandler(13, temp1, readFrom, temp2, right, errno);

	res = read(readFrom, right, rHalf*sizeof(uint32_t));
	if (res != rHalf*sizeof(uint32_t))
		errorHandler(14, temp1, readFrom, temp2, right, errno);

	qsort(right, rHalf, sizeof(uint32_t), cmp);
	res = write(temp2, right, rHalf*sizeof(uint32_t));
	if (res != rHalf*sizeof(uint32_t))
		errorHandler(15, temp1, readFrom, temp2, right, errno);

	close(readFrom);
	lseek(temp1, 0, SEEK_SET);
	lseek(temp2, 0, SEEK_SET);
	
	int writeTo = open(argv[1], O_TRUNC | O_WRONLY);
	if (writeTo == -1)
		errorHandler(16, writeTo, temp1, temp2, right, errno);

	free(right);
	
	uint32_t leftNum;
	uint32_t rightNum;

	if (read(temp1, &leftNum, sizeof(leftNum)) != sizeof(leftNum))
		errorHandler(17, writeTo, temp1, temp2, NULL, errno);

	if (read(temp2, &rightNum, sizeof(rightNum)) != sizeof(rightNum))
		errorHandler(18, writeTo, temp1, temp2, NULL, errno);

	uint32_t leftCnt = 0;
	uint32_t rightCnt = 0;

	while (leftCnt < lHalf && rightCnt < rHalf)
	{
		if (leftNum < rightNum)
		{
			if (write(writeTo, &leftNum, sizeof(leftNum)) != sizeof(leftNum))
				errorHandler(19, writeTo, temp1, temp2, NULL, errno);

			if (read(temp1, &leftNum, sizeof(leftNum)) != sizeof(leftNum))
				break;

			leftCnt++;	
		}
		else
		{

			if (write(writeTo, &rightNum, sizeof(rightNum)) != sizeof(rightNum))
				errorHandler(20, writeTo, temp1, temp2, NULL, errno);

			if (read(temp2, &rightNum, sizeof(rightNum)) != sizeof(rightNum))
				break;

			rightCnt++;
		}
	}

	while (read(temp1, &leftNum, sizeof(leftNum)) == sizeof(leftNum))
	{
		if (write(writeTo, &leftNum, sizeof(leftNum)) != sizeof(leftNum))
			errorHandler(22, writeTo, temp1, temp2, NULL, errno);
	}

	while (read(temp2, &rightNum, sizeof(rightNum)) == sizeof(rightNum))
	{
		if (write(writeTo, &rightNum, sizeof(rightNum)) != sizeof(rightNum))
			errorHandler(23, writeTo, temp1, temp2, NULL, errno);
	}

	close(temp1);
	close(temp2);
	close(writeTo);
	exit(0);
}
