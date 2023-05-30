main:
	@python parser.py ebay_data/items-*.json
	@sqlite3 ebay_data.db < create.sql
	@sqlite3 ebay_data.db < load.txt
	@echo "\nSuccess generating ./ebay_data.db"
	@rm -f AuctionUser.dat
	@rm -f Auction.dat
	@rm -f Bid.dat
	@rm -f Classification.dat
	@rm -f Item.dat

clean: 
	@rm -f AuctionUser.dat
	@rm -f Auction.dat
	@rm -f Bid.dat
	@rm -f Classification.dat
	@rm -f Item.dat

test:
	@python parser.py ebay_data/items-*.json
	@sqlite3 ebay_data.db < create.sql
	@sqlite3 ebay_data.db < load.txt
	@echo "\nSuccess generating ./ebay_data.db"
	@rm -f AuctionUser.dat
	@rm -f Auction.dat
	@rm -f Bid.dat
	@rm -f Category.dat
	@rm -f Classification.dat
	@echo "\nQuery-1\n-------"
	@sqlite3 ebay_data.db < query1.sql
	@echo "\nQuery-2\n-------"
	@sqlite3 ebay_data.db < query2.sql
	@echo "\nQuery-3\n-------"
	@sqlite3 ebay_data.db < query3.sql
	@echo "\nQuery-4\n-------"
	@sqlite3 ebay_data.db < query4.sql
	@echo "\nQuery-5\n-------"
	@sqlite3 ebay_data.db < query5.sql
	@echo "\nQuery-6\n-------"
	@sqlite3 ebay_data.db < query6.sql
	@echo "\nQuery-7\n-------"
	@sqlite3 ebay_data.db < query7.sql
	@echo "\nExpected: {13422, 80, 8365, 1046871451, 3130, 6717, 150}"
	@rm -f AuctionUser.dat
	@rm -f Auction.dat
	@rm -f Bid.dat
	@rm -f Classification.dat
	@rm -f Item.dat
	@rm -f ebay_data.db

