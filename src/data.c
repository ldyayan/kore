/*!
 * \file data.c
 *
 * \par Copyright
 * Copyright (C) 1997-2018 Heroes of Kore
 *
 * \par CirlcleMUD copyright
 * Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
 *
 * \par DikuMUD copyright
 * CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991
 *
 * \par License
 * All rights reserved. See license.doc for complete information.
 *
 * \author Geoffrey Davis
 * \addtogroup data
 */
#define _DATA_C_

#include "structs.h"
#include "utils.h"
#include "buffer.h"
#include "data.h"

/*!
 * Constructs a new data element.
 * \addtogroup data
 * \return the new data element
 * \sa DataFree(Data*)
 */
Data *DataAlloc(void) {
  Data *d = NULL;
  CREATE(d, Data, 1);
  return (d);
}

/*!
 * Clears a data element.
 * \addtogroup data
 * \param d the data element to clear
 */
static void DataClear(Data *d) {
  if (!d) {
    log("invalid `d` Data");
  } else {
    register size_t j = 0;
    for (j = 0; j < d->entryListSize; ++j) {
      free(d->entryList[j].key);
      DataFree(d->entryList[j].value);
    }
    if (d->value)
      free(d->value);
    if (d->entryList)
      free(d->entryList);
    memset(d, '\0', sizeof(Data));
  }
}

/*!
 * Frees a data element.
 * \addtogroup data
 * \param d the data element to free
 * \sa DataAlloc()
 */
void DataFree(Data *d) {
  if (d) {
    DataClear(d);
    free(d);
  }
}

/*!
 * Searches for an entry and returns its value.
 * \addtogroup data
 * \param d the data element to search
 * \param key the key identifying the entry whose value to return
 * \return the value of the entry indicated by the specified key or NULL
 */
const Data *DataGet(
	const Data *d,
	const char *key) {
  register const Data *result = NULL;
  if (d && *IF_STR(key) != '\0') {
    register size_t j = 0;
    for (j = 0; !result && j < d->entryListSize; ++j) {
      if (str_cmp(d->entryList[j].key, key) == 0)
        result = d->entryList[j].value;
    }
  }
  return (result);
}

/*!
 * Searches for an entry and returns its value.
 * \addtogroup data
 * \param d the data element to search
 * \param key the key identifying the entry whose value to return
 * \param nameList the newline-terminated list of names
 * \param defaultValue the value to return if the specified key
 *     cannot be resolved, or the value of the entry indicated by
 *     the specified key cannot be converted to the desired type
 * \return the value of the entry indicated by the specified key
 *     or the specified default value
 */
unsigned long DataGetBits(
	const Data *d,
	const char *key,
	const char *nameList[],
	const unsigned long defaultValue) {
  register unsigned long result = defaultValue;
  if (!nameList) {
    log("invalid `nameList` string array");
  } else {
    register const Data *found = DataGet(d, key);
    if (found) {
      register size_t j = 0;
      for (j = 0, result = 0; *nameList[j] !=  '\n'; ++j) {
        REMOVE_BIT(result, 1 << j);
        if (DataGetYesNo(found, nameList[j], FALSE))
          SET_BIT(result, 1 << j);
      }
    }
  }
  return (result);
}

/*!
 * Searches for an entry and returns its value.
 * \addtogroup data
 * \param d the data element to search
 * \param key the key identifying the entry whose value to return
 * \param format the sscanf-style format specifier
 * \return the number of values successfully scanned or -1
 */
ssize_t DataGetFormatted(
        const Data *d,
        const char *key,
        const char *format, ...) {
  register ssize_t result = -1;
  register const Data *found = DataGet(d, key);
  if (found && *IF_STR(found->value) != '\0') {
    va_list args;
    va_start(args, format);
    result = vsscanf(found->value, format, args);
    va_end(args);
  }
  return (result);
}

/*!
 * Searches for an entry and returns its value.
 * \addtogroup data
 * \param d the data element to search
 * \param key the key identifying the entry whose value to return
 * \param defaultValue the value to return if the specified key
 *     cannot be resolved, or the value of the entry indicated by
 *     the specified key cannot be converted to the desired type
 * \return the value of the entry indicated by the specified key
 *     or the specified default value
 */
double DataGetNumber(
	const Data *d,
	const char *key,
	const double defaultValue) {
  double result = defaultValue;
  if (DataGetFormatted(d, key, " %lg ", &result) != 1) {
    result = defaultValue;
  }
  return (result);
}

/*!
 * Searches for an entry and returns its value.
 * \addtogroup data
 * \param d the data element to search
 * \param key the key identifying the entry whose value to return
 * \param defaultValue the value to return if the specified key
 *     cannot be resolved, or the value of the entry indicated by
 *     the specified key cannot be converted to the desired type
 * \return the value of the entry indicated by the specified key
 *     or the specified default value
 */
const char *DataGetString(
	const Data *d,
	const char *key,
	const char *defaultValue) {
  register const char *result = defaultValue;
  register const Data *found = DataGet(d, key);
  if (found && found->value) {
    result = IF_STR(found->value);
  }
  return (result);
}

/*!
 * Searches for an entry and returns its value.
 * \addtogroup data
 * \param d the data element to search
 * \param key the key identifying the entry whose value to return
 * \param defaultValue the value to return if the specified key
 *     cannot be resolved, or the value of the entry indicated by
 *     the specified key cannot be converted to the desired type
 * \return the value of the entry indicated by the specified key
 *     or the specified default value
 */
char *DataGetStringCopy(
	const Data *d,
	const char *key,
	const char *defaultValue) {
  register char *result = (char*) defaultValue;
  register const Data *found = DataGet(d, key);
  if (found && found->value) {
      result = IF_STR(found->value);
  }
  if (result)
    result = strdup(result);

  return (result);
}

/*!
 * Searches for an entry and returns its value.
 * \addtogroup data
 * \param d the data element to search
 * \param key the key identifying the entry whose value to return
 * \param defaultValue the value to return if the specified key
 *     cannot be resolved, or the value of the entry indicated by
 *     the specified key cannot be converted to the desired type
 * \return the value of the entry indicated by the specified key
 *     or the specified default value
 */
time_t DataGetTime(
	const Data *d,
	const char *key,
	const time_t defaultValue) {
  register time_t result = defaultValue;

  struct tm time;
  memset(&time, '\0', sizeof(struct tm));

  /* Scan time fields from the value */
  const ssize_t N = DataGetFormatted(d, key,
        " %d-%d-%d %d:%d:%d %d ",
        &time.tm_year, &time.tm_mon, &time.tm_mday,
        &time.tm_hour, &time.tm_min, &time.tm_sec,
        &time.tm_isdst);

  /* Valid when 3, 6, or 7 values provided */
  if (N == 3 || N == 6 || N == 7) {
    time.tm_mon  -= 1;    /* Month must be 0-11 */
    time.tm_year -= 1900; /* Year must be 114 for 2014 */
    result = mktime(&time);
  }
  return (defaultValue);
}

/*!
 * Searches for an entry and returns its value.
 * \addtogroup data
 * \param d the data element to search
 * \param key the key identifying the entry whose value to return
 * \param nameList the newline-terminated list of names
 * \param defaultValue the value to return if the specified key
 *     cannot be resolved, or the value of the entry indicated by
 *     the specified key cannot be converted to the desired type
 * \return the value of the entry indicated by the specified key
 *     or the specified default value
 */
ssize_t DataGetType(
	const Data *d,
	const char *key,
	const char *nameList[],
	const ssize_t defaultValue) {
  if (!nameList) {
    log("invalid `nameList` string array");
  } else {
    const char *found = DataGetString(d, key, NULL);
    if (*IF_STR(found) != '\0') {
      register size_t j = 0;
      for (j = 0; *nameList[j] != '\n'; ++j) {
        if (str_cmp(nameList[j], found) == 0)
          return (j);
      }
      char logmessg[MAX_STRING_LENGTH] = {'\0'};
      snprintf(logmessg, sizeof(logmessg), "Key `%s` has unknown type `%s`", key, found);
      log(logmessg);
    }
  }
  return (defaultValue);
}

/*!
 * Searches for an entry and returns its value.
 * \addtogroup data
 * \param d the data element to search
 * \param key the key identifying the entry whose value to return
 * \param defaultValue the value to return if the specified key
 *     cannot be resolved, or the value of the entry indicated by
 *     the specified key cannot be converted to the desired type
 * \return the value of the entry indicated by the specified key
 *     or the specified default value
 */
bool DataGetYesNo(
	const Data *d,
	const char *key,
	const bool defaultValue) {
  register bool result = defaultValue;
  register const char *found = DataGetString(d, key, NULL);
  if (*IF_STR(found) != '\0') {
    double value = 0.0;
    if (sscanf(found, " %lg ", &value) == 1) {
      result = /* NaN */ value == value && value != 0.0;
    } else {
      /* Check for `Yes` */
      if (str_cmp("Y", found) == 0 || str_cmp("Yes", found) == 0)
        result = TRUE;

      /* Check for `No` */
      if (str_cmp("N", found) == 0 || str_cmp("No", found) == 0)
        result = FALSE;
    }
  }
  return (result);
}

/*!
 * Returns the key of the entry at the specified index.
 * \addtogroup data
 * \param d the data element
 * \param index the zero-based index of the entry whose key to return
 * \return the key of the entry at the specified index or NULL
 * \sa DataValueAt(const Data*, const size_t)
 */
const char *DataKeyAt(
        const Data *d,
        const size_t index) {
  register const char *keyAt = NULL;
  if (d && index < d->entryListSize) {
    keyAt = IF_STR(d->entryList[index].key);
  }
  return (keyAt);
}

/* Some local prototypes */
static Data *DataReadString(FILE *stream);
static Data *DataReadStringBlock(FILE *stream);
static Data *DataReadStruct(FILE *stream);
static bool DataReadStructKey(
	char *key, const size_t keylen,
	FILE *stream);
static Data *DataReadStructValue(FILE *stream);

/*!
 * Reads a string from a stream.
 * \addtogroup value
 * \param stream the stream from which to read a string
 * \return a new string, or NULL
 */
Data *DataReadString(FILE *stream) {
  register Data *d = NULL;
  if (!stream) {
    log("invalid `stream` FILE");
  } else {
    char messg[MAX_STRING_LENGTH] = {'\0'};
    register size_t messgpos =  0;

    while (!d) {
      int ch = fgetc(stream);
      if (ch == EOF)
	break;

      if (ch == '~') {
	ch = fgetc(stream);
	if (ch == '~') {
	  BPrintf(messg, sizeof(messg), messgpos, "%c", ch);
	} else {
	  while (ch != EOF && ch != '\n' && isspace(ch)) {
	    ch = fgetc(stream);
	  }
	  if (ch == EOF || ch == '\n') {
	    d = DataAlloc();
	    d->value = strdup(messg);
	  }
	}
      } else
	BPrintf(messg, sizeof(messg), messgpos, "%c", ch);
    }
    if (!d) {
      char logmessg[MAX_STRING_LENGTH] = {'\0'};
      snprintf(logmessg, sizeof(logmessg), "/DATA/ Unexpected EOF while reading string: %s", messg);
      log(logmessg);
    }
  }
  return (d);
}

/*!
 * Reads a string from a stream.
 * \addtogroup data
 * \param stream the stream from which to read
 * \return a new string, or NULL
 */
Data *DataReadStringBlock(FILE *stream) {
  register Data *d = NULL;
  if (!stream) {
    log("invalid 'stream' FILE");
  } else {
    char messg[MAX_STRING_LENGTH] = {'\0'};
    register size_t messgpos = 0;

    while (!d) {
      register int ch = fgetc(stream);
      if (ch == EOF)
	break;

      if (ch == '~') {
	ch = fgetc(stream);
	if (ch == '~') {
	  BPrintf(messg, sizeof(messg), messgpos, "%c", ch);
	} else {
	  while (ch != EOF && ch != '\n' && isspace(ch))
            ch = fgetc(stream);
	
	  if (ch != EOF && ch != '\n') {
	    log("/DATA/ Missing EOF or EOL while parsing string block");
	    break;
	  }

	  /* Queue of string block lines */
	  const char **lines = NULL;
	  register size_t linesSize = 0;
	  CREATE(lines, const char*, 1);
	  lines[linesSize++] = messg;

	  /* Enqueue string block lines */
	  register const char *ptr = NULL;
	  for (ptr = messg; *ptr != '\0'; ++ptr) {
	    if (*ptr == '\n') {
	      RECREATE(lines, const char*, linesSize + 1);
	      lines[linesSize++] = ptr + 1;
	    }
	  }

	  /* Count least number of leading spaces */
	  register size_t fewestSpaces = -1;
	  register size_t j = 0;
	  for (j = 0; j < linesSize; ++j) {
	    register size_t currentSpaces = 0;
	    for (ptr = lines[j]; *ptr != '\0' && *ptr != '\n'; ++ptr) {
	      if (isspace(*ptr) && *ptr != '\r') {
		++currentSpaces;
	      } else if (*ptr != '\r')
		break;
	    }
	    fewestSpaces = MIN(fewestSpaces, currentSpaces);
	  }

	  /* Reconstruct the string block */
	  if (fewestSpaces && fewestSpaces != -1) {
	    register size_t wmessgpos = 0;
	    register size_t j = 0;
	    for (j = 0; j < linesSize; ++j) {
	      for (ptr = lines[j] + fewestSpaces; *ptr != '\0'; ++ptr) {
		if (*ptr == '\n')
		  BPrintf(messg, sizeof(messg), wmessgpos, "\r");
		if (*ptr != '\r')
		  BPrintf(messg, sizeof(messg), wmessgpos, "%c", *ptr);
		if (*ptr == '\n')
		  break;
	      }
	    }
	  }

	  /* Cleanup line queue */
	  if (lines)
	    free(lines);

	  /* Create the data element */
	  d = DataAlloc();
	  d->value = strdup(messg);
	}
      } else if (ch != '\r') {
	if (ch == '\n')
	  BPrintf(messg, sizeof(messg), messgpos, "\r");
	BPrintf(messg, sizeof(messg), messgpos, "%c", ch);
      }
    }
  }
  return (d);
}

/*!
 * Reads a struct key from a stream.
 * \addtogroup data
 * \param stream the stream from which to read
 * \return a new struct key or NULL
 */
bool DataReadStructKey(
	char *key, const size_t keylen,
	FILE *stream) {
  register bool result = FALSE;
  if (!key && keylen) {
    log("invalid `key` buffer");
  } else if (!stream) {
    log("invalid `stream` FILE");
  } else {
    register size_t keypos = 0;
    for (result = TRUE; result; ) {
      const int ch = fgetc(stream);
      if (ch == EOF) {
	char logmessg[MAX_STRING_LENGTH] = {'\0'};
	snprintf(logmessg, sizeof(logmessg), "/DATA/ Unexpected EOF while parsing structure key: %s", key);
	log(logmessg);
	result = FALSE;
      } else if (ch == ':') {
	if (keypos == 0) {
	  log("/DATA/ Unexpected colon while parsing structure key");
	  result = FALSE;
	} else
	  break;
      } else if (isalnum(ch) || ch == '_' || ch == '$') {
	BPrintf(key, keylen, keypos, "%c", ch);
      } else {
	result = FALSE;
      }
    }
  }
  return (result);
}

/*!
 * Reads a struct from a stream.
 * \addtogroup data
 * \param stream the stream from which to read
 * \return a new struct, or NULL
 */
Data *DataReadStruct(FILE *stream) {
  register Data *d = NULL;
  if (!stream) {
    log("invalid 'stream' FILE");
  } else {
    while (TRUE) {
      int ch = fgetc(stream);
      if (ch == EOF)
	break;
      if (ch == '~')
	break;
      if (isalnum(ch) || ch == '_' || ch == '$') {
	if (ungetc(ch, stream) != ch) {
	  char logmessg[MAX_STRING_LENGTH] = {'\0'};
	  snprintf(logmessg, sizeof(logmessg), "/DATA/ ungetc() failed: errno=%d", errno);
	  log(logmessg);
	  DataFree(d), d = NULL;
	  break;
	}
	char key[MAX_INPUT_LENGTH] = {'\0'};
	if (!DataReadStructKey(key, sizeof(key), stream)) {
	  log("/DATA/ Couldn't parse structure key");
	  DataFree(d), d = NULL;
	  break;
	}
	Data *value = DataReadStructValue(stream);
	if (!value) {
	  log("/DATA/ Couldn't parse structure value");
	  DataFree(d), d = NULL;
	  break;
	}
	if (!d)
	  d = DataAlloc();

	if (DataPut(d, key, value) != value) {
	  char logmessg[MAX_STRING_LENGTH] = {'\0'};
	  snprintf(logmessg, sizeof(logmessg), "/DATA/ Couldn't add structure value: %s", key);
	  log(logmessg);
	  DataFree(d), d = NULL;
	  break;
	}
      } else if (!isspace(ch))
	break;
    }
  }
  return (d);
}

/*!
 * Reads a data element from a stream.
 * \addtogroup data
 * \param stream the stream from which to read
 * \return a new data element, or NULL
 */
Data *DataReadStructValue(FILE *stream) {
  register Data *d = NULL;
  if (!stream) {
    log("invalid `stream` FILE");
  } else {
    register int ch = fgetc(stream);
    if (ch == EOF) {
      log("/DATA/ Unexpected EOF while reading structure value");
    } else if (ch == '-') {
      ch = fgetc(stream);
      while (ch != EOF && ch != '\n' && isspace(ch))
	ch = fgetc(stream);

      if (ch == EOF) {
	log("/DATA/ Unexpected EOF while reading structure value");
      } else if (ch != '\n') {
	log("/DATA/ Missing EOL while reading structure value");
      } else {
	d = DataReadStringBlock(stream);
      }
    } else {
      while (ch != EOF && ch != '\n' && isspace(ch))
	ch = fgetc(stream);

      if (ch == EOF) {
	log("/DATA/ Unexpected EOF while reading structure value");
      } else if (ch == '\n') {
	const int chSaved = ch;
	const long position = ftell(stream);
	while (isspace(ch) && ch != EOF)
	  ch = fgetc(stream);
	fseek(stream, position, SEEK_SET);
	ch = chSaved;
	d = DataReadStruct(stream);
      } else {
	if (ungetc(ch, stream) != ch) {
	  char logmessg[MAX_STRING_LENGTH] = {'\0'};
	  snprintf(logmessg, sizeof(logmessg), "/DATA/ ungetc() failed: errno=%d", errno);
	  log(logmessg);
	} else
	  d = DataReadString(stream);
      }
    }
    if (!d)
      log("/DATA/ Error while reading structure value");
  }
  return (d);
}

/*!
 * Loads a data element.
 * \addtogroup data
 * \param filename the filename of the file to read
 * \return a data element representing the contents of
 *     the file indicated by the specified filename, or NULL
 * \sa DataLoadStream(FILE*)
 */
Data *DataLoadFile(const char *filename) {
  register Data *loaded = NULL;
  if (*IF_STR(filename) == '\0') {
    log("invalid `filename` string");
  } else {
    FILE *stream = fopen(filename, "rt");
    if (!stream) {
      char logmessg[MAX_STRING_LENGTH] = {'\0'};
      snprintf(logmessg, sizeof(logmessg), "Could not open file %s for reading", filename);
      log(logmessg);
    } else {
      loaded = DataLoadStream(stream);
      fclose(stream);
    }
  }
  return (loaded);
}

/*!
 * Loads a data element.
 * \addtogroup data
 * \param stream the stream to read
 * \return a data element representing the contents of
 *     the specified stream, or NULL
 * \sa DataLoadFile(const char*)
 */
Data *DataLoadStream(FILE *stream) {
  register Data *loaded = NULL;
  if (!stream) {
    log("invalid `stream` FILE");
  } else {
    loaded = DataReadStruct(stream);
  }
  return (loaded);
}

/*!
 * Inserts or updates an entry into a data element.
 * \addtogroup data
 * \param d the data element into which to insert an entry
 * \param key the key that identifies the entry to insert or update
 * \param value the value of the entry identified by the specified key
 * \return the new of updated data element or NULL
 */
Data *DataPut(
	Data *d,
	const char *key,
	Data *value) {
  if (!d) {
    log("invalid `d` Data");
    value = NULL;
  } else if (*IF_STR(key) == '\0') {
    log("invalid `key` string");
    value = NULL;
  } else if (!value) {
    log("invalid `value` Data");
  } else {
    /* Make a copy of the key */
    char realKey[MAX_INPUT_LENGTH] = {'\0'};
    strlcpy(realKey, key, sizeof(realKey));

    /* Find the next highest index */
    if (strcmp(realKey, "%") == 0) {
      register size_t highest = 0;
      if (d->entryListSize) {
	register size_t j = 0;
	for (j = 0; j < d->entryListSize; ++j) {
	  size_t N = 0;
	  if (sscanf(DataKeyAt(d, j), " %zu ", &N) == 1)
	    highest = MAX(N, highest);
	}
      }
      snprintf(realKey, sizeof(realKey), "%zu", highest + 1);
    }

    /* Search for the entry */
    register size_t j = 0;
    for (j = 0; j < d->entryListSize; ++j) {
      if (str_cmp(d->entryList[j].key, realKey) == 0)
        break;
    }

    /* Create a new entry if the entry is not found */
    if (j == d->entryListSize) {
      RECREATE(d->entryList, DataEntry, d->entryListSize + 1);
      d->entryList[d->entryListSize].key = NULL;
      d->entryList[d->entryListSize].value = NULL;
      d->entryListSize++;
    }
    if (d->entryList[j].key)
      free(d->entryList[j].key);
    d->entryList[j].key = strdup(realKey);
    DataFree(d->entryList[j].value);
    d->entryList[j].value = value;
  }
  return (value);
}

/*!
 * Inserts or updates an entry into a data element.
 * \addtogroup data
 * \param d the data element into which to insert an entry
 * \param key the key that identifies the entry to insert or update
 * \param nameList the newline-terminated list of names
 * \param value the value of the entry identified by the specified key
 * \return the new of updated data element or NULL
 */
Data *DataPutBits(
	Data *d,
	const char *key,
	const char *nameList[],
	const unsigned long value) {
  register Data *result = NULL;
  if (!d) {
    log("invalid `d` Data");
  } else if (!nameList) {
    log("invalid `nameList` string array");
  } else {
    if (*IF_STR(key) == '\0') {
      result = d;
    } else {
      result = DataAlloc();
      if (DataPut(d, key, result) != result) {
        DataFree(result);
        result = NULL;
      }
    }
    if (result) {
      register size_t j = 0;
      for (j = 0; *nameList[j] != '\n'; ++j) {
	if (IS_SET(value, 1 << j) != 0)
	  DataPutYesNo(result, nameList[j], TRUE);
      }
      DataSort(result);
    }
  }
  return (result);
}

/*!
 * Inserts or updates an entry into a data element.
 * \addtogroup data
 * \param d the data element into which to insert an entry
 * \param key the key that identifies the entry to insert or update
 * \param format the printf-style format specifier
 * \return the new of updated data element or NULL
 */
Data *DataPutFormatted(
	Data *d,
	const char *key,
	const char *format, ...) {
  register Data *result = NULL;
  if (!d) {
    log("invalid `d` Data");
  } else {
    va_list args;
    va_start(args, format);
    char messg[MAX_STRING_LENGTH] = {'\0'};
    const ssize_t N = snprintf(messg, sizeof(messg), format, args);
    if (N >= 0 && N < sizeof(messg) - 1)
      result = DataPutString(d, key, messg);
    va_end(args);
  }
  return (result);
}

/*!
 * Inserts or updates an entry into a data element.
 * \addtogroup data
 * \param d the data element into which to insert an entry
 * \param key the key that identifies the entry to insert or update
 * \param value the value of the entry identified by the specified key
 * \return the new of updated data element or NULL
 */
Data *DataPutNumber(
	Data *d,
	const char *key,
	const double value) {
  register Data *result = NULL;
  if (!d) {
    log("invalid `d` Data");
  } else {
    result = DataPutFormatted(d, key, "%g", value);
  }
  return (result);
}

/*!
 * Inserts or updates an entry into a data element.
 * \addtogroup data
 * \param d the data element into which to insert an entry
 * \param key the key that identifies the entry to insert or update
 * \param value the value of the entry identified by the specified key
 * \return the new of updated data element or NULL
 */
Data *DataPutString(
	Data *d,
	const char *key,
	const char *value) {
  register Data *result = NULL;
  if (!d) {
    log("invalid `d` Data");
  } else {
    if (*IF_STR(key) == '\0') {
      result = d;
    } else {
      result = DataAlloc();
      if (DataPut(d, key, result) != result) {
	DataFree(result);
	result = NULL;
      }
    }
    if (result) {
      DataClear(result);
      if (result->value)
	free(result->value);
      result->value = value ? strdup(value) : NULL;
    }
  }
  return (result);
}

/*!
 * Inserts or updates an entry into a data element.
 * \addtogroup data
 * \param d the data element into which to insert an entry
 * \param key the key that identifies the entry to insert or update
 * \param value the value of the entry identified by the specified key
 * \return the new of updated data element or NULL
 */
Data *DataPutTime(
	Data *d,
	const char *key,
	const time_t value) {
  register Data *result = NULL;
  if (!d) {
    log("invalid `d` Data");
  } else {
    struct tm time;
    if (localtime_r(&value, &time) != &time) {
      char logmessg[MAX_STRING_LENGTH] = {'\0'};
      snprintf(logmessg, sizeof(logmessg), "localtime_r() failed: errno=%d", errno);
      log(logmessg);
    } else {
      result = DataPutFormatted(d, key,
        "%-4.4d/%-2.2d/%-2.2d %-2.2d:%-2.2d:%-2.2d %d",
        time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,
        time.tm_hour, time.tm_min, time.tm_sec,
        time.tm_isdst);
    }
  }
  return (result);
}

/*!
 * Inserts or updates an entry into a data element.
 * \addtogroup data
 * \param d the data element into which to insert an entry
 * \param key the key that identifies the entry to insert or update
 * \param nameList the newline-terminated list of names
 * \param value the value of the entry identified by the specified key
 * \return the new of updated data element or NULL
 */
Data *DataPutType(
	Data *d,
	const char *key,
	const char *nameList[],
	const ssize_t value) {
  register Data *result = NULL;
  if (!d) {
    log("invalid `d` Data");
  } else if (!nameList) {
    log("invalid `nameList` string array");
  } else {
    register size_t j = 0;
    for (j = 0; !result && *nameList[j] != '\n'; j++) {
      if (j == value)
        result = DataPutString(d, key, nameList[j]);
    }
    if (!result && *nameList[j] == '\n')
      result = DataPutNumber(d, key, j);
  }
  return (result);
}

/*!
 * Inserts or updates an entry into a data element.
 * \addtogroup data
 * \param d the data element into which to insert an entry
 * \param key the key that identifies the entry to insert or update
 * \param value the value of the entry identified by the specified key
 * \return the new of updated data element or NULL
 */
Data *DataPutYesNo(
	Data *d,
	const char *key,
	const bool value) {
  register Data *result = NULL;
  if (!d) {
    log("invalid `d` Data");
  } else {
    result = DataPutString(d, key, value ? "Yes" : "No");
  }
  return (result);
}

/* Some local prototypes */
static bool DataWriteIndent(FILE *stream, const size_t indent);
static bool DataWriteString(FILE *stream, const size_t indent, const Data *d);
static bool DataWriteStruct(FILE *stream, const size_t indent, const Data *d);
static bool DataWriteStructKey(FILE *stream, const char *key);
static bool DataWriteStructValue(FILE *stream, const size_t indent, const Data *d);

/*!
 * Writes indentation to a stream.
 * \addtogroup data
 * \param stream the stream to which to write
 * \param indent the indentation level
 * \return TRUE if the number of spaces indicated by the
 *     specified indentation level were successfully written
 *     to the specified stream
 */
static bool DataWriteIndent(
	FILE *stream,
	const size_t indent) {
  bool result = FALSE;
  if (!stream) {
    log("invalid `stream` FILE");
  } else {
    const int spaces = (int) indent * 2;
    if (fprintf(stream, "%*s", spaces, "") == spaces)
      result = TRUE;
  }
  return (result);
}

/*!
 * Writes a structure key to a stream.
 * \addtogroup data
 * \param stream the stream to which to write
 * \param key the key to write to the specified stream
 * \return TRUE if the specified key was successfully written
 */
static bool DataWriteStructKey(
	FILE *stream,
	const char *key) {
  bool result = FALSE;
  if (!stream) {
    log("invalid `stream` FILE");
  } else if (*IF_STR(key) == '\0') {
    log("invalid `key` string");
  } else {
    register const char *ptr = key;
    for (result = TRUE; result && *ptr != '\0'; ++ptr) {
      if (result && fputc(*ptr, stream) == EOF)
	result = FALSE;
    }
    if (result && fputc(':', stream) == EOF)
      result = FALSE;
  }
  return (result);
}

/*!
 * Writes a scalar to a stream.
 * \addtogroup data
 * \param stream the stream to which to write
 * \param indent the indentation level
 * \param d the scalar to write to the stream
 * \return TRUE if the scalar was successfully written
 */
static bool DataWriteString(
	FILE *stream,
	const size_t indent,
	const Data *d) {
  register bool result = FALSE;
  if (!stream) {
    log("invalid `stream` FILE");
  } else if (!d) {
    log("invalid `d` Data");
  } else {
    register const char *ptr = IF_STR(d->value);
    if (strchr(ptr, '\n') != NULL) {
      result = DataWriteIndent(stream, indent);
    } else {
      result = TRUE;
      while (*ptr != '\0' && isspace(*ptr))
	++ptr;
    }
    for (; result && *ptr != '\0'; ptr++) {
      if (result && *ptr != '\r' && fputc(*ptr, stream) == EOF)
	result = FALSE;
      if (result && *ptr == '~' && fputc(*ptr, stream) == EOF)
	result = FALSE;
      if (result && *ptr == '\n')
	result = DataWriteIndent(stream, indent);
    }
    if (result && fprintf(stream, "~\n") != 2)
      result = FALSE;
  }
  return (result);
}

/*!
 * Writes a struct to a stream.
 * \addtogroup data
 * \param stream the stream to which to write
 * \param indent the indentation level
 * \param d the struct to write to the stream
 * \return TRUE if the struct was successfully written
 */
static bool DataWriteStruct(
	FILE *stream,
	const size_t indent,
	const Data *d) {
  register bool result = FALSE;
  if (!stream) {
    log("invalid `stream` FILE");
  } else if (!d) {
    log("invalid `d` Data");
  } else {
    result = TRUE;
    if (d->entryListSize) {
      register size_t j = 0;
      for (j = 0; result && j < d->entryListSize; ++j) {
	if (result)
	  result = DataWriteIndent(stream, indent);
	if (result) {
	  const char *keyAt = DataKeyAt(d, j);
	  result = DataWriteStructKey(stream, keyAt);
	}
	if (result) {
	  const Data *valueAt = DataValueAt(d, j);
	  result = DataWriteStructValue(stream, indent + 1, valueAt);
	}
      }
    }
    if (result)
      result = DataWriteIndent(stream, indent);
    if (result && fprintf(stream, "~\n") != 2)
      result = FALSE;
  }
  return (result);
}

/*!
 * Writes a struct value to a stream.
 * \addtogroup data
 * \param stream the stream to which to write
 * \param indent the indentation level
 * \param value the value to write to the stream
 * \return TRUE if the specified value was successfully written
 */
static bool DataWriteStructValue(
	FILE *stream,
	const size_t indent,
	const Data *d) {
  register bool result = FALSE;
  if (!stream) {
    log("invalid `stream` FILE");
  } else if (!d) {
    log("invalid `d` Data");
  } else {
    if (d->entryListSize) {
      if (fputc('\n', stream) != EOF)
	result = TRUE;
      if (result)
	result = DataWriteStruct(stream, indent, d);
    } else {
      if (strchr(IF_STR(d->value), '\n') != NULL) {
	if (fputc('-', stream) != EOF)
	  result = TRUE;
	if (result && fputc('\n', stream) == EOF)
	  result = TRUE;
      } else {
	if (fputc(' ', stream) != EOF)
	  result = TRUE;
      }
      if (result)
	result = DataWriteString(stream, indent, d);
    }
  }
  return (result);
}

/*!
 * Saves a data element.
 * \addtogroup data
 * \param d the data element to save
 * \param fname the filename of the file to write
 * \return TRUE if the file indicated by the specified filename
 *     was successfully written
 * \sa DataSaveStream(const Data*, FILE*)
 */
bool DataSaveFile(
	const Data *d,
	const char *fname) {
  register bool result = FALSE;
  if (!d) {
    log("invalid `d` Data");
  } else if (*IF_STR(fname) == '\0') {
    log("invalid `fname` string");
  } else {
    char tempfname[PATH_MAX] = {'\0'};
    if (snprintf(tempfname, sizeof(tempfname), "%s.tmp", fname) > 0) {
      FILE *stream = fopen(tempfname, "wt");
      if (!stream) {
	char logmessg[MAX_STRING_LENGTH] = {'\0'};
	snprintf(logmessg, sizeof(logmessg), "Couldn't open file `%s` for writing", tempfname);
        log(logmessg);
      } else {
        result = DataSaveStream(d, stream);
        fclose(stream);

	char logmessg[MAX_STRING_LENGTH] = {'\0'};
        if (result && rename(tempfname, fname) != 0) {
	  snprintf(logmessg, sizeof(logmessg), "rename() failed: errno=%d", errno);
          log(logmessg);
	}
        if (unlink(tempfname) != 0 && errno != ENOENT) {
          snprintf(logmessg, sizeof(logmessg), "unlink() failed: errno=%d", errno);
          log(logmessg);
	}
      }
    }
  }
  return (result);
}

/*!
 * Saves a data element.
 * \addtogroup data
 * \param d the data element to save
 * \param stream the stream to which to write
 * \return TRUE if the specified stream was successfully written
 * \sa DataSaveFile(const Data*, const char*)
 */
bool DataSaveStream(
	const Data *d,
	FILE *stream) {
  register bool result = FALSE;
  if (!stream) {
    log("invalid `stream` FILE");
  } else {
    result = DataWriteStruct(stream, 0, d);
  }
  return (result);
}

/*!
 * Gets the length of a data element.
 * \addtogroup data
 * \param d the data element whose length to return
 * \return the length of the specified data element or zero
 */
size_t DataSize(const Data *d) {
  register size_t result = 0;
  if (!d) {
    log("invalid `d` Data");
  } else {
    result = d->entryListSize;
  }
  return (result);
}

/*!
 * Compares two data element entries.
 * \addtogroup data
 * \param x the first data element entry to compare
 * \param y the second data element entry to compare
 * \return < 0 if the first entry precedes the second entry;
 *         > 0 if the first entry follows the second entry;
 *           0 if the two data element entries are equal
 */
static int DataSortProc(const void *x, const void *y) {
  const DataEntry *xEntry = (const DataEntry*) x;
  const DataEntry *yEntry = (const DataEntry*) y;
  return str_cmp(xEntry->key, yEntry->key);
}

/*!
 * Sorts a data element.
 * \addtogroup data
 * \param d the data element whose keys to sort
 */
void DataSort(Data *d) {
  if (!d) {
    log("invalid `d` Data");
  } else {
    qsort(d->entryList,
	  d->entryListSize,
	  sizeof(DataEntry),
	  DataSortProc);
  }
}

/*!
 * Returns the value of the entry value at the specified index.
 * \addtogroup data
 * \param d the data element
 * \param index the zero-based index of the entry whose value to return
 * \return the value of the entry at the specified index or NULL
 * \sa DataKeyAt(const Data*, const size_t)
 */
Data *DataValueAt(
	const Data *d,
	const size_t index) {
  register Data *valueAt = NULL;
  if (d && index < d->entryListSize) {
    valueAt = d->entryList[index].value;
  }
  return (valueAt);
}
