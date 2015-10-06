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
                    row_headers[this_location] = []
                row_headers[this_location].append(item)
                next(row_iter)
    print row_headers
    return row_headers


def extract_data(row, field_to_locations, headers):
    row_data = {}
    rowdate = parse_date(row[0], row[1])
    this_location = ''
    row_iter = iter(row)
    next(row_iter)  # date
    next(row_iter)  # time
    count = 2
    inner_count = 0
    for item in row_iter:
        count += 1
        if count in field_to_locations:
            inner_count = 0
            this_location = field_to_locations[count]
            if this_location not in row_data:
                row_data[this_location] = {"date": rowdate}
        try:
            next_item = next(row_iter)
        except StopIteration:
            break
        this_field = headers[this_location][inner_count]
        inner_count += 1
        if len(item) == 0 or len(next_item) == 0:
            value = 0
            status = ""
            units = ""
        else:
            value = float(item)
            status = next_item[0]
            units = next_item[2:]

        row_data[this_location][this_field] = {
            "value": value,
            "status": status,
            "units": units,
        }
    return row_data


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
            # Here be the clever bit. We should be able to
            # extract things we want now.
            data = extract_data(row, field_to_locations, headers)
            print(data)
