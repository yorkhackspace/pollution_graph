#!/usr/bin/env python3

import csv
import logging
from logging import debug, warn, error, critical
import matplotlib.pyplot as plt
import numpy as np
from datetime import datetime, timedelta

#logging.basicConfig(level=logging.DEBUG)

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
    debug (rowdatetime)
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

    debug(field_to_locations)
    return field_to_locations


def parse_headers(row, field_to_locations):
    # This is not quite there as it needs the locations from above
    this_location = ""
    debug(row)
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
        count += 1
    debug (row_headers)
    print (row_headers)
    return row_headers


def extract_row_data(row, field_to_locations, headers):
    """ Extract a row into a array of dict
    { 'location':'Gillygate'
      'date': datetime
      'gas': 'foo'
      'value': nnnn
      'unit': 'mmm'
      'status': 'P'
    }
    """
    row_data = []
    this_location = ''

    row_date = parse_date(row[0], row[1])

    row_iter = iter(row)
    next(row_iter)  # date
    next(row_iter)  # time

    count = 2
    inner_count = 0
    for item in row_iter:
        count += 1
        # Work out the location of this field
        if count in field_to_locations:
            inner_count = 0
            this_location = field_to_locations[count]

        try:
            next_item = next(row_iter)
        except StopIteration:
            # If we reach the end of the line
            break
        this_gas = headers[this_location][inner_count % 2]
        inner_count += 1
        if len(item) == 0 or len(next_item) == 0:
            value = 0
            status = ""
            units = ""
        else:
            value = float(item)
            status = next_item[0]
            units = next_item[2:]
            count += 2

        row_data.append({
            'date': row_date,
            'location': this_location,
            'gas': this_gas,
            'value': value,
            'status': status,
            'units': units,
        })

    return row_data


def extract_csv_to_list(csvfilename):
    """ Returns an array of all the rows
    """
    data = []
    with open(csvfilename) as f:
        reader = csv.reader(f)
        line = 0
        for row in reader:
            line += 1
            if line < 3:
                continue
            elif line == 3:
                field_to_locations = parse_locations(row)
                continue
            elif line == 4:
                headers = parse_headers(row, field_to_locations)
                continue
            elif len(row) > 2:
                debug(row)
                row_data = extract_row_data(row, field_to_locations, headers)
                # I don't think this is very clever or efficient
                data += row_data

        debug(data)
        return data

if __name__ == "__main__":
    data = extract_csv_to_list('input.csv')
    print ( data[0] )
    print ( data[1] )
    print ( data[2] )

    start_time = datetime.strptime("21/09/2015 00:00:00", "%d/%m/%Y %H:%M:%S")
    end_time = datetime.strptime("22/09/2015 00:00:00", "%d/%m/%Y %H:%M:%S")

    location_data = [ x for x in data if x['location'] == 'York Bootham' ]
    location_day = [x for x in location_data if ( x['date'] >= start_time and x['date'] < end_time ) ]
    print(location_day)
    location_data_for_gas = [(x['date'], x['value'] ) for x in location_day if x['gas'] == 'NO2']

    dates = [x[0] for x in location_data_for_gas]
    opens = [x[1] for x in location_data_for_gas]

    plt.plot_date(dates, opens)
    plt.show()
