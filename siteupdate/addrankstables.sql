
DROP TABLE IF EXISTS travelerMileageStats;
DROP TABLE IF EXISTS clinchedActiveStats;
DROP TABLE IF EXISTS clinchedActivePreviewStats;

CREATE TABLE travelerMileageStats (
    traveler VARCHAR(255) PRIMARY KEY,
    totalActiveMileage DECIMAL(10,2),
    rankActiveMileage INT,
    clinchedActiveMileage DECIMAL(10,2),
    activePercentage DECIMAL(5,2),
    totalActivePreviewMileage DECIMAL(10,2),
    rankActivePreviewMileage INT,
    clinchedActivePreviewMileage DECIMAL(10,2),
    activePreviewPercentage DECIMAL(5,2)
);

-- Insert the data into the new table
INSERT INTO travelerMileageStats (
    traveler, totalActiveMileage, rankActiveMileage, clinchedActiveMileage, activePercentage,
    totalActivePreviewMileage, rankActivePreviewMileage, clinchedActivePreviewMileage, activePreviewPercentage
)
WITH TotalMileage AS (
    SELECT 
        ROUND(SUM(activeMileage), 2) AS totalActiveMileage,
        ROUND(SUM(activePreviewMileage), 2) AS totalActivePreviewMileage
    FROM overallMileageByRegion
),
RankedMileage AS (
    SELECT 
        co.traveler,
        tm.totalActiveMileage,
        ROUND(SUM(COALESCE(co.activeMileage, 0)), 2) AS clinchedActiveMileage,
        CASE 
            WHEN tm.totalActiveMileage = 0 THEN 0
            ELSE ROUND(SUM(COALESCE(co.activeMileage, 0)) / tm.totalActiveMileage * 100, 2)
        END AS activePercentage,
        tm.totalActivePreviewMileage,
        ROUND(SUM(COALESCE(co.activePreviewMileage, 0)), 2) AS clinchedActivePreviewMileage,
        CASE 
            WHEN tm.totalActivePreviewMileage = 0 THEN 0
            ELSE ROUND(SUM(COALESCE(co.activePreviewMileage, 0)) / tm.totalActivePreviewMileage * 100, 2)
        END AS activePreviewPercentage,
        le.includeInRanks
    FROM 
        clinchedOverallMileageByRegion co
    CROSS JOIN 
        TotalMileage tm
    LEFT JOIN 
        overallMileageByRegion o ON co.region = o.region
    JOIN 
        listEntries le ON co.traveler = le.traveler
    WHERE 
        co.traveler IS NOT NULL
    GROUP BY 
        co.traveler, tm.totalActiveMileage, tm.totalActivePreviewMileage, le.includeInRanks
),
RankedIncluded AS (
    SELECT
        traveler,
        RANK() OVER (ORDER BY clinchedActiveMileage DESC) AS rankActiveMileage,
        RANK() OVER (ORDER BY clinchedActivePreviewMileage DESC) AS rankActivePreviewMileage
    FROM 
        RankedMileage
    WHERE 
        includeInRanks = 1
)
SELECT 
    rm.traveler, 
    rm.totalActiveMileage, 
    COALESCE(ri.rankActiveMileage, -1) AS rankActiveMileage, 
    rm.clinchedActiveMileage, 
    rm.activePercentage,
    rm.totalActivePreviewMileage, 
    COALESCE(ri.rankActivePreviewMileage, -1) AS rankActivePreviewMileage, 
    rm.clinchedActivePreviewMileage, 
    rm.activePreviewPercentage
FROM 
    RankedMileage rm
LEFT JOIN 
    RankedIncluded ri ON rm.traveler = ri.traveler;


CREATE TABLE clinchedActiveStats AS
WITH ActiveRoutes AS (
    SELECT COUNT(cr.route) AS total
    FROM connectedRoutes AS cr
    LEFT JOIN systems ON cr.systemName = systems.systemName
    WHERE systems.level = 'active'
),
TravelerStats AS (
    SELECT
        ccr.traveler,
        COUNT(ccr.route) AS driven,
        SUM(ccr.clinched) AS clinched,
        (SELECT total FROM ActiveRoutes) AS activeRoutes,
        ROUND(COUNT(ccr.route) / (SELECT total FROM ActiveRoutes) * 100, 2) AS drivenPercent,
        ROUND(SUM(ccr.clinched) / (SELECT total FROM ActiveRoutes) * 100, 2) AS clinchedPercent
    FROM
        connectedRoutes AS cr
    LEFT JOIN
        clinchedConnectedRoutes AS ccr ON cr.firstRoot = ccr.route
    LEFT JOIN
        routes ON ccr.route = routes.root
    LEFT JOIN
        systems ON routes.systemName = systems.systemName
    WHERE
        systems.level = 'active'
    GROUP BY
        ccr.traveler
)
SELECT
    ts.*,
    RANK() OVER (ORDER BY ts.driven DESC) AS drivenRank,
    RANK() OVER (ORDER BY ts.clinched DESC) AS clinchedRank
FROM TravelerStats ts;

CREATE TABLE clinchedActivePreviewStats AS
WITH ActivePreviewRoutes AS (
    SELECT COUNT(cr.route) AS total
    FROM connectedRoutes AS cr
    LEFT JOIN systems ON cr.systemName = systems.systemName
    WHERE systems.level = 'active' OR systems.level = 'preview'
),
TravelerStats AS (
    SELECT
        ccr.traveler,
        COUNT(ccr.route) AS driven,
        SUM(ccr.clinched) AS clinched,
        (SELECT total FROM ActivePreviewRoutes) AS activePreviewRoutes,
        ROUND(COUNT(ccr.route) / (SELECT total FROM ActivePreviewRoutes) * 100, 2) AS drivenPercent,
        ROUND(SUM(ccr.clinched) / (SELECT total FROM ActivePreviewRoutes) * 100, 2) AS clinchedPercent
    FROM
        connectedRoutes AS cr
    LEFT JOIN
        clinchedConnectedRoutes AS ccr ON cr.firstRoot = ccr.route
    LEFT JOIN
        routes ON ccr.route = routes.root
    LEFT JOIN
        systems ON routes.systemName = systems.systemName
    WHERE
        systems.level = 'active' OR systems.level = 'preview'
    GROUP BY
        ccr.traveler
)
SELECT
    ts.*,
    RANK() OVER (ORDER BY ts.driven DESC) AS drivenRank,
    RANK() OVER (ORDER BY ts.clinched DESC) AS clinchedRank
FROM TravelerStats ts;
