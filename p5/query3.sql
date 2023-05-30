-- Query 3
SELECT COUNT (*)
	FROM (
		SELECT COUNT(*) AS 'cnt'
    			FROM Classification 
    			GROUP BY itemID
	)
	WHERE cnt = 4;  
