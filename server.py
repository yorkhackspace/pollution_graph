#!/usr/bin/env python

# Read air quality CSV, then start a serial console server to
# query that data based on site, date and time.
# XXX There's a bug in the parsing : some sites have different parameters to
# others, e.g. Holgate's first column is PM10.
# Should really be checking the header columns for each site, too.

import csv
#import serial
import sys

site_columns = {}
site_names = []
site_data = {}
max_val = 0
max_ts = None

with open("input.csv", "rb") as infile:
    reader = csv.reader(infile)
    for rownum, row in enumerate(reader):
        if rownum == 2:
            print("row 2, reader line num:", reader.line_num)
            for colnum, col in enumerate(row):
                if colnum > 1 and len(col) > 0:
                    site_names.append(col)
                    site_columns[col] = colnum
                    site_data[col] = {}
        elif rownum > 3 and len(row) > 2:
            # Data starts here for real.
            # First 2 cols are date and time.  I guess we'll store as strings for now.
            # We could actually just store this stuff as row number, if we want to
            # serialise to SD card files for the arduino later.
            dat = row[0]
            tim = row[1]
            timestamp = "%s_%s" % (dat, tim)
            # Assume that we only want the NO figures, which are in the first
            # column for each site.
            for sitename, col in site_columns.items():
                value = row[col]
                if len(value) > 0:
                    value = float(value)
                else:
                    value = 0
                site_data[sitename][timestamp] = value
                if value > max_val:
                    max_val = value
                    max_ts = timestamp

# To test, let's check the data for some sample points

print("Max val:", max_val)
print("Bootham 2 sep 09:00:", site_data["York Bootham"]["02/09/2015_09:00:00"])
print("Bootham 8 sep 09:00:", site_data["York Bootham"]["08/09/2015_09:00:00"])
print("Nunnery 5 sep 13:00:", site_data["York Nunnery Lane"]["05/09/2015_13:00:00"])

# current max value

print("Holgate 8 sep 11:00:", site_data["York Holgate"]["08/09/2015_11:00:00"])

# And now, start a server to provide this data.  We'll normalise against the

# max value seen, to get a percentage figure of the max.

#ser = serial.Serial('/dev/ttyACM0', 57600)


# Let's make a dummy data file.
print """
// Data structure for the input data.
// We store as an array of locations, each with data from hour 05:00 to hour 23:00.
// Each is a percentage of the global maximum in the dataset as a whole.
byte dataPoints[][19] = 
{
"""

# XXX determine the best date, preferably
chosen_date = "08/09/2015"

# print site_data.keys()

sites = (
    'DUMMY',
    'DUMMY',
    'York Heworth Green', # 2
    'DUMMY', # Fishergate marker, but no data
    'York Nunnery Lane', # 4
    'York Lawrence Street', #5
    'York Holgate',  # 6
    'York Gillygate', # 7 : but data is all zeros
    'York Bootham', #8

)

for site in sites:
    print "  {\n  // " + site + "\n"

    vals = []
    if site == 'DUMMY':
        for hr in range(5,24):
            vals.append("0")
    else:
        sd = site_data[site]
        # print repr(sd)
        for hr in range(5,24):
            ts = chosen_date + ("_%02d:00:00" % hr)
            # print ts
            data = sd[ts]
            # print repr(data)
            perc = int(data * 100 / max_val)
            vals.append(str(perc))
        
    print ", ".join(vals)

    print "  },\n"

print """
};
"""

exit(0)

while 1:
    # line = ser.readline()
#    line = sys.stdin.readline()
    # Expect request lines in the format: 'R 1,05,13'
    # to read site 1, 5th of month, 13:00
    if line[0] == 'R':
        components = line[2:].split(',')
        try:
            site = int(components[0])
            day = int(components[1])
            hour = int(components[2])
            print("Parsed: site:%s,day:%d,hour:%d" % (site, day, hour))
            site_name = site_names[site]
            timestamp = "%02d/09/2015_%02d:00:00" % (day, hour)
            data = site_data[site_name][timestamp]
            print("data: %r" % data)
            percent = int((data / max_val) * 100)
            print("normalised against %.3f = %d" % (max_val, percent))
#            ser.write('! %03d\x0a' % percent)
#            print('! %03d\x0a' % percent)
        except Exception as e:
            print("Exception:%r - ignored.", e)

