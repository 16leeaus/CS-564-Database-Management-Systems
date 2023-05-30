-- Query 5
SELECT COUNT (*)
	FROM AuctionUser AS 'a'
	WHERE (
		a.seller = 'True'
		AND a.rating > 1000
	);
