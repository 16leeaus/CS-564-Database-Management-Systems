# -*- coding: utf-8 -*-
'''
Name:       Jacob Dillie
Student ID: 9079059573
Wisc ID:    dillie@wisc.edu
Class:      Comp Sci 564 
Semester:   Spring 2022
Project:    Assignment 5
File:       parser.py

1) Directory handling -- the parser takes a list of eBay json files
and opens each file inside of a loop. You just need to fill in the rest.
2) Dollar value conversions -- the json files store dollar value amounts in
a string like $3,453.23 -- we provide a function to convert it to a string
like XXXXX.xx.
3) Date/time conversions -- the json files store dates/ times in the form
Mon-DD-YY HH:MM:SS -- we wrote a function (transformDttm) that converts to the
for YYYY-MM-DD HH:MM:SS, which will sort chronologically in SQL.

Your job is to implement the parseJson function, which is invoked on each file 
by the main function. We create the initial Python dictionary object of items 
for you; the rest is up to you!
Happy parsing!
'''

import sys
from json import loads
from re import sub

columnSeparator = "|"

# Dictionary of months used for date transformation
MONTHS = {'Jan': '01', 'Feb': '02', 'Mar': '03', 'Apr': '04', 'May': '05', 'Jun': '06',
          'Jul': '07', 'Aug': '08', 'Sep': '09', 'Oct': '10', 'Nov': '11', 'Dec': '12'}

# AuctionUser
UserID = []
UserName = []
UserHexCode = []
Bidder = []
Seller = []
Rating = []
Location = []
Country = []

# Auction
AuctionID1 = []
ItemID3 = []
NumBids = []
StartPrice = []
CurrPrice = []
SellerID = []
StartTime = []
EndTime = []

# Bid
BidID = []
AuctionID2 = []
BidderID = []
BidVal = []
TimeStamp = []

# Classification
ClassID = []
ItemID2 = []
CategoryName = []

# Item
ItemID1 = []
ItemName = []

'''
Returns true if a file ends in .json
'''


def isJson(f):
    return len(f) > 5 and f[-5:] == '.json'


'''
Converts month to a number, e.g. 'Dec' to '12'
'''


def transformMonth(mon):
    if mon in MONTHS:
        return MONTHS[mon]
    else:
        return mon


'''
Transforms a timestamp from Mon-DD-YY HH:MM:SS to YYYY-MM-DD HH:MM:SS
'''


def transformDttm(dttm):
    dttm = dttm.strip().split(' ')
    dt = dttm[0].split('-')
    date = '20' + dt[2] + '-'
    date += transformMonth(dt[0]) + '-' + dt[1]
    return date + ' ' + dttm[1]


'''
Transform a dollar value amount from a string like $3,453.23 to XXXXX.xx
'''


def transformDollar(money):
    if money == None or len(money) == 0:
        return money
    return sub(r'[^\d.]', '', money)


'''
Converts string to hex
'''


def hexStr(string):
    hexString = "0x"
    for i in (string.encode('ascii')):
        hexString += (hex(ord(i))[2:])
    return hexString


'''
Insert Attributes into Item Table
'''


def processItem(item):
    # Keep Track of Metrics
    itemID = int(item['ItemID'], 10)

    # Item Table
    # Add each unique item
    # if (itemID not in ItemID1):
    ItemID1.append(itemID)
    ItemName.append(item['Name'])

    # Classification Table
    # Add each unique category, itemID pair
    i = 1
    for cat1 in item['Category']:
        j = i
        for cat2 in item['Category'][i:]:
            # For a matching pair, set one to Nonetype and then pass over
            if (cat1 != None and cat2 != None and cat1 == cat2):
                item['Category'][j] = None
            j += 1
        i += 1
        if (cat1 != None):
            ClassID.append(len(ClassID))
            ItemID2.append(itemID)
            CategoryName.append(cat1)

    # AuctionUser Table
    # Add each unique user. For users that already exist,
    # update fields
    # Sellers
    hexID = hexStr(item['Seller']['UserID'])
    if (hexID not in UserHexCode):
        UserID.append(len(UserID))
        UserName.append(item['Seller']['UserID'])
        UserHexCode.append(hexID)
        Bidder.append(False)
        Seller.append(True)
        Rating.append(item['Seller']['Rating'])
        Location.append(item['Location'])
        if (item['Country'] != None):
            Country.append(item['Country'])
        else:
            Country.append('NULL')
    else:
        Seller[UserHexCode.index(hexID)] = True
    # Bidders
    if (item['Number_of_Bids'] != 0 and item['Bids'] != None):
        for bid in item['Bids']:
            hexID = hexStr(bid['Bid']['Bidder']['UserID'])
            if (hexID not in UserHexCode):
                UserID.append(len(UserID))
                UserName.append(item['Seller']['UserID'])
                UserHexCode.append(hexID)
                Bidder.append(True)
                Seller.append(False)
                Rating.append(bid['Bid']['Bidder']['Rating'])
                # Location and Country may not be populated, so we will
                # set to NULL as default
                if 'Location' in (bid['Bid']['Bidder']):
                    Location.append(bid['Bid']['Bidder']['Location'])
                else:
                    Location.append('NULL')
                if ('Country' in bid['Bid']['Bidder']):
                    Country.append(bid['Bid']['Bidder']['Country'])
                else:
                    Country.append('NULL')
            else:
                Bidder[UserHexCode.index(hexID)] = True

    # Auction Table
    # Item, auction, seller, number of bids, start price, highest price,
    # start time, and end time
    AuctionID1.append(len(AuctionID1))
    ItemID3.append(itemID)
    SellerID.append(UserID[UserHexCode.index(hexID)])
    NumBids.append(int(item['Number_of_Bids'], 10))
    startPrice = 0.0
    if (item['First_Bid'] != None):
        startPrice = transformDollar(item['First_Bid'])
    StartPrice.append(startPrice)
    CurrPrice.append(transformDollar(item['Currently']))
    StartTime.append(transformDttm(item['Started']))
    EndTime.append(transformDttm(item['Ends']))

    # Bid Table
    # Auction, bidder, value, timestamp
    if (item['Number_of_Bids'] != 0 and item['Bids'] != None):
        for bid in item['Bids']:
            BidID.append(len(BidID))
            AuctionID2.append(AuctionID1[len(AuctionID1) - 1])
            BidderID.append(UserID[UserHexCode.index(hexStr(
                bid['Bid']['Bidder']['UserID']))])
            BidVal.append(transformDollar(bid['Bid']['Amount']))
            TimeStamp.append(transformDttm(bid['Bid']['Time']))

    # End
    return


'''
Parses a single json file by iterating over each item and inserting attributes
into tables
'''


def parseJson(json_file):
    with open(json_file, 'r') as f:
        # creates a Python dictionary of Items
        items = loads(f.read())['Items']
        # for the supplied json file
        # Item Table
        for item in items:
            processItem(item)
            pass


'''
Writes data to .dat files
'''


def writeDat():
    # AuctionUser: UserID, UserName, Bidder, Seller, Rating, Location, Country
    f = open('./AuctionUser.dat', 'w')
    for i in range(0, len(UserID)):
        line = str(UserID[i]) + '|\"' + str(UserName[i]).replace('\"', '\"\"')
        line += '\"|'
        if (Bidder[i]):
            line += 'True' + '|'
        else:
            line += 'False' + '|'
        if (Seller[i]):
            line += 'True' + '|'
        else:
            line += 'False' + '|'
        line += (str(Rating[i]) + '|')
        if (Location[i] == 'NULL'):
            line += 'NULL'
        else:
            line += ('\"' + Location[i].replace('\"', '\"\"') + '\"')
        line += ('|' + Country[i] + '\n')
        f.write(line)
    f.close()
    print('\nSuccess writing ./AuctionUser.dat')

    # Auction: AuctionID1, ItemID3, SellerID, NumBids, StartPrice
    # CurrPrice, StartTime, EndTime
    f = open('./Auction.dat', 'w')
    for i in range(0, len(AuctionID1)):
        f.write(str(AuctionID1[i]) + '|' + str(ItemID3[i]) + '|'
                + str(SellerID[i]) + '|' + str(NumBids[i]) + '|'
                + str(StartPrice[i]) + '|' + str(CurrPrice[i]) + '|'
                + str(StartTime[i]) + '|' + str(EndTime[i]) + '\n')
    f.close()
    print('Success writing ./Auction.dat')

    # Bid: BidID, AuctionID2, BidderID, BidVal, TimeStamp
    f = open('./Bid.dat', 'w')
    for i in range(0, len(AuctionID2)):
        f.write(str(BidID[i]) + '|' + str(AuctionID2[i]) + '|' + str(BidderID[i]) + '|'
                + str(BidVal[i]) + '|' + str(TimeStamp[i]) + '\n')
    f.close()
    print('Success writing ./Bid.dat')

    # Classification: ClassID, ItemID2, CategoryName
    f = open('./Classification.dat', 'w')
    for i in range(0, len(ClassID)):
        f.write(str(ClassID[i]) + '|' + str(ItemID2[i]) + '|\"'
                + str(CategoryName[i]).replace('\"', '\"\"') + '\"\n')
    f.close()
    print('Success writing ./Classification.dat')

    # Items: ItemID1, ItemName
    f = open('./Item.dat', 'w')
    for i in range(0, len(ItemID1)):
        f.write(str(ItemID1[i]) + '|\"' + ItemName[i].replace('\"', '\"\"')
                + '\"\n')
    f.close()
    print('Success writing ./Item.dat')

    return


'''
Loops through each json files provided on the command line and passes each file
to the parser
'''


def main(argv):
    # check arguments
    if len(argv) < 2:
        sys.stderr.write('Usage: python skeleton_json_parser.py <path to json'
                         + ' files>')
        sys.exit(1)

    # loops over all .json files in the argument
    for f in argv[1:]:
        if isJson(f):
            parseJson(f)
            print("Success parsing " + str(f))

    # write stored data
    writeDat()


if __name__ == '__main__':
    main(sys.argv)
