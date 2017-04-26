/*************************************************************************
	SQL Server Database - Stored Procedure Creation Script
	
	The following script creates the stored procedures required for the
	sample SQL Server program.
*************************************************************************/

/*************************************************************************
	YOU MUST BE LOGGED ON AS 'SA' TO RUN THIS SCRIPT
*************************************************************************/

/*************************************************************************
	USE THE USER_DB DATABASE
*************************************************************************/
USE USER_DB
GO

/*************************************************************************
	CREATE USER_DB DATABASE STORED PROGRAMS
*************************************************************************/
DROP PROCEDURE STUDENT_ADMIN.SHOW_RELUCTANT_STUDENTS
GO

CREATE PROCEDURE STUDENT_ADMIN.SHOW_RELUCTANT_STUDENTS AS
SELECT FNAME+' '+LNAME+', social security number '+SSN+' is not enrolled in any classes!'
FROM STUDENT_ADMIN.STUDENT S
WHERE NOT EXISTS
	(SELECT 'X' FROM STUDENT_ADMIN.GRADE G WHERE G.SSN = S.SSN)
ORDER BY SSN
RETURN @@ROWCOUNT 
GO

GRANT EXECUTE ON STUDENT_ADMIN.SHOW_RELUCTANT_STUDENTS TO DATA_ADMIN, USER_LOGON
GO

DROP PROCEDURE STUDENT_ADMIN.CLASSES_NEEDED_TO_GRADUATE
GO

CREATE PROCEDURE STUDENT_ADMIN.CLASSES_NEEDED_TO_GRADUATE @VSSN VARCHAR(9) AS
SELECT 
	SSN, FNAME+' '+LNAME AS NAME, MAJOR,
	CCODE, CNAME, DEPT,
	ISNULL(ROOM,'******') ROOM,
	ISNULL(PREREQ,'****') PREREQ
FROM STUDENT_ADMIN.STUDENT S1, DEPT_ADMIN.CLASS C
WHERE  
NOT EXISTS
(select 'X'
from student_admin.student s2, student_admin.grade g
where s2.ssn = g.ssn
  and s2.ssn = s1.ssn
  and g.ccode = c.ccode)
AND DEPT = MAJOR
AND SSN LIKE @VSSN
ORDER BY S1.SSN, C.CCODE
GO

GRANT EXECUTE ON STUDENT_ADMIN.CLASSES_NEEDED_TO_GRADUATE
	TO DATA_ADMIN, USER_LOGON
GO

DROP PROCEDURE DEPT_ADMIN.SHOW_EMPTY_CLASSES
GO

CREATE PROCEDURE DEPT_ADMIN.SHOW_EMPTY_CLASSES
AS
SELECT C.CCODE, C.CNAME, C.DEPT,
	ISNULL(C.ROOM,'******') ROOM, ISNULL(C.PREREQ,'****') PREREQ
FROM DEPT_ADMIN.CLASS C WHERE NOT EXISTS
(SELECT 'X' FROM STUDENT_ADMIN.GRADE G
WHERE G.CCODE = C.CCODE)
GO

GRANT EXECUTE ON DEPT_ADMIN.SHOW_EMPTY_CLASSES TO DATA_ADMIN, USER_LOGON
GO

DROP PROCEDURE DEPT_ADMIN.DELETE_DEPT
GO

CREATE PROCEDURE DEPT_ADMIN.DELETE_DEPT @VDEPT VARCHAR(4) AS
DELETE FROM DEPT_ADMIN.DEPT WHERE DEPT = @VDEPT
RETURN @@ROWCOUNT
GO

GRANT EXECUTE ON DEPT_ADMIN.DELETE_DEPT TO DATA_ADMIN
GO

DROP PROCEDURE DEPT_ADMIN.INSERT_DEPT
GO

CREATE PROCEDURE DEPT_ADMIN.INSERT_DEPT
	@VDEPT VARCHAR(4), @VDNAME VARCHAR(30) AS
INSERT INTO DEPT_ADMIN.DEPT (DEPT,DNAME)
VALUES (@VDEPT, @VDNAME)
RETURN @@ROWCOUNT
GO

GRANT EXECUTE ON DEPT_ADMIN.INSERT_DEPT TO DATA_ADMIN
GO

DROP PROCEDURE DEPT_ADMIN.DELETE_CLASS
GO

CREATE PROCEDURE DEPT_ADMIN.DELETE_CLASS @VCCODE VARCHAR(4) AS
DELETE FROM DEPT_ADMIN.CLASS WHERE CCODE = @VCCODE
RETURN @@ROWCOUNT
GO

GRANT EXECUTE ON DEPT_ADMIN.DELETE_CLASS TO DATA_ADMIN
GO

DROP PROCEDURE DEPT_ADMIN.INSERT_CLASS
GO

CREATE PROCEDURE DEPT_ADMIN.INSERT_CLASS
	@VCCODE VARCHAR(4), @VCNAME VARCHAR(30), @VDEPT VARCHAR(4),
	@VROOM VARCHAR(6), @VPREREQ VARCHAR(4) AS
INSERT INTO DEPT_ADMIN.CLASS (CCODE, CNAME, DEPT, ROOM, PREREQ)
VALUES (@VCCODE, @VCNAME, @VDEPT, @VROOM, @VPREREQ)
RETURN @@ROWCOUNT
GO

GRANT EXECUTE ON DEPT_ADMIN.INSERT_CLASS TO DATA_ADMIN
GO