#pragma once

#ifndef DISTRIBUTED_TRANSACTION_H
#define DISTRIBUTED_TRANSACTION_H

#include "Student.h"

typedef struct distributed_transaction_t {
	unsigned int id;
	Student student;
} DistributedTransaction;

#endif // !DISTRIBUTED_TRANSACTION_H

