#!/usr/bin/env python3

import csv
from datetime import datetime, timedelta


def parse_date(rowdate, rowtime):
    add_day = False
    # Cope with the fact that they use hour 24,
    # which is the next day!
    if rowtime.startswith("24"):
        rowtime = "00{}".format(rowtime[2:])
        add_day = True
    datetimestring = "{} {}".format(rowdate, rowtime)
    rowdatetime = datetime.strptime(datetimestring, "%d/%m/%Y %H:%M:%S")
    if add_day:
        rowdatetime = rowdatetime + timedelta(hours=24)
    print (rowdatetime)
    return rowdatetime


def parse_locations(row):
    field_to_locations = {}
    row_iter = iter(row)
    # Skip the random "Data"
    next(row_iter)
    count = 1
    for item in row_iter:
        count += 1
        if len(item) > 0:
            field_to_locations[count] = item

    print(field_to_locations)
    return field_to_locations


def parse_headers(row, field_to_locations):
    # This is not quite there as it needs the locations from above
    this_location = ""
    print(row)
    row_headers = {}
    at_start = True
    count = 1
    row_iter = iter(row)
    for item in row_iter:
        count += 1
        if count in field_to_locations:
            this_location = field_to_locations[count]
        if at_start:
            at_start = False
            next(row_iter)
            continue
        else:
            if len(item) > 0:
                if this_location not in row_headers:
                    row_headers[this_location] = {}
                row_headers[this_location][item] = next(row_iter)
    print row_headers
    return row_headers

with open('input.csv') as f:
    reader = csv.reader(f)
    line = 0
    for row in reader:
        line += 1
        if line < 3:
            continue
        if line == 3:
            field_to_locations = parse_locations(row)
            continue
        if line == 4:
            headers = parse_headers(row, field_to_locations)
            continue
        if len(row) > 2:
            print(row)
            rowdate = parse_date(row[0], row[1])
            # Here be the clever bit. We should be able to
            # extract things we want now.
