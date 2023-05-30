-- Query 7
SELECT COUNT(DISTINCT categoryName)
	FROM Auction AS 'a', Bid AS 'b', Classification AS 'c'
	Where (
		b.value > 100
		AND a.auctionID = b.auctionID
		AND c.itemID = a.itemID
	);
