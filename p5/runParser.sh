# Remove generated files
rm -f AuctionUser.dat
rm -f Auction.dat
rm -f Bid.dat
rm -f Classification.dat
rm -f Item.dat

# Create .dat files for SQL Loading
python parser.py ebay_data/items-*.json
