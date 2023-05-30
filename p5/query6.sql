-- Query 6
SELECT COUNT (*)
	FROM AuctionUser AS 'a'
	WHERE (
		a.seller = 'True'
		AND a.bidder = 'True'
	)
