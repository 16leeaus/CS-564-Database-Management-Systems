DROP TABLE IF EXISTS Item;
DROP TABLE IF EXISTS Classification;
DROP TABLE IF EXISTS Auction;
DROP TABLE IF EXISTS AuctionUser;
DROP TABLE IF EXISTS Bid;

CREATE TABLE Item (
	itemID 		INT 		NOT NULL,
	itemName 	TEXT		NOT NULL,
	PRIMARY KEY (itemID)
);

CREATE TABLE Classification (
	classID 	INT		NOT NULL	UNIQUE,
	itemID 		INT		NOT NULL,
	categoryName 	TEXT		NOT NULL,
	PRIMARY KEY (classID),
	FOREIGN KEY (itemID) 	REFERENCES Auction(itemID)
);

CREATE TABLE Auction (
	auctionID 	INT		NOT NULL	UNIQUE,
	itemID	 	INT		NOT NULL	UNIQUE,
	sellerID 	INT		NOT NULL,
	numBids 	INT		NOT NULL,
	startPrice 	DOUBLE		NOT NULL,
	currPrice 	DOUBLE		NOT NULL,
	startTime 	DATETIME	NOT NULL,
        endTime 	DATETIME	NOT NULL,
	PRIMARY KEY (auctionID),
	FOREIGN KEY (itemID) 	REFERENCES Item(itemID),
	FOREIGN KEY (sellerID) 	REFERENCES AuctionUser(userID)
);

CREATE TABLE AuctionUser (
	userID 		INT		NOT NULL	UNIQUE,
	userName 	TEXT		NOT NULL,
	bidder 		VARCHAR(6) 	NOT NULL,
	seller 		VARCHAR(6)	NOT NULL,
	rating 		INT		NOT NULL,
	location 	TEXT,
	country 	TEXT,
	PRIMARY KEY (userID)
);

CREATE TABLE Bid (
	bidID	 	INT		NOT NULL	UNIQUE,
	auctionID 	INT		NOT NULL,
	bidderID 	INT		NOT NULL,
	value 		DOUBLE		NOT NULL,
	timeStmp 	DATETIME	NOT NULL,
	PRIMARY KEY (bidID),
	FOREIGN KEY (auctionID) REFERENCES Auction(auctionID),
	FOREIGN KEY (bidderID) 	REFERENCES AuctionUser(userID)
);


