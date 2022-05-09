Title:    OSRC Explained
Author:   Jonathan Rupp
Date:     03-May-2002
License:  Copyright (C) 2002 LightSys Technology Services.  See LICENSE.txt.
-------------------------------------------------------------------------------

The osrc has the following JS object variables used for housekeeping purposes.

    readahead (number)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Set by the C code of this module.  This is the minumum number of
        records to be requested from the server when the osrc moves.
        This is to cut down on connection overhead.

    scrollahead (number)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Set by the C code of this module.  This is the minumum number of
        records to be requested from the server when the osrc scrolls.  This
        is to cut down on connection overhead.

    replicasize (number)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Set by the C code of this module.  This is the size of the internal
        replica.  When the current record moves outside of this window, the
        window needs to slide in that direction by 'readahead' records.

    sql (string)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Set by the C code of this module.  This is the base sql query, with
        only a SELECT clause.  This is used later to build the real queries to
        use.

    filter (string)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Set by the C code of this module.  This is the base WHERE criteria
        (with no WHERE in it).  This is used later to build the real queries
        to use.

    children (object)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Dynamically created in javascript as child forms/tables/etc.
        register with the OSRC.  Each object under this must impliment all
        osrc/form interaction functions.

    init (boolean)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This is set to true when doing an initial query, false otherwise.  I
        don't think this is actually used, but I'm not sure....

    pendingqueryobject (object)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This is an object representing the query that is waiting to be run.
        This is used so that the current query is not overwritten if one of
        the forms cancels the transition to this new query.

    pendingquery (string)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This temporarily hold the pending query while child forms decide
        whether to cancel the transition to this query.

    pending (boolean)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This flag indicates whether the osrc is currently waiting for a
        callback, either from a child form, or from the onload event of the
        hidden layer used for communication with the server.  The OSRC will
        (silently) ignore all movement and re-query request while in this
        state.

    query (string)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This holds the current query, escaped, just as it was sent to the
        server in the OSML openquery request.

    queryobject (object)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This object holds the current filter criteria that the osrc is
        using.  This is used when a table child asks for the table to be
        sorted, so that the current WHERE clause of the query can be
        preserved without having to parse the SQL stored in query.

    orderobject (object)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This object holds the current sort order.  This is used to allow the
        sorting by multiple columns.  When the table wants to sort by a
        certain column, it grabs a copy of this, modifies it, then passes it
        to the OSRC as the parameter to ActionOrderObject.

    pendingorderobject (object)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This is like orderobject, except that is for the pending re-query.
        It will be copied to orderobject on the successful completion of the
        query.

    TargetRecord (number)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This is used by the OSRC to know what record is it's target to
        include in the replica.  It will stop fetching records once it is in
        the replica.  If end of query is hit before the OSRC gets to this
        record number, the OSRC will stop and set TargetRecord to be
        LastRecord.

    CurrentRecord (number)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This is the record denoted as 'current'.  This record is NOT
        guaranteed to be currently in the replica.  If the end of query is
        hit and this number is larger than LastRecord, CurrentRecord is set
        to LastRecord.

    OSMLRecord (number)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This is the next record that the server will give us (via OSML) if
        we do a OSML queryfetch.  This allows us to keep our place in the
        recordset.

    LastRecord (number)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This is the record number of the last record in the replica.

    FirstRecord(number)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This is the record number of the first record in the replica.

    sid (string)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This is the current session identifier.

    qid (string)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This is the current query identifier.

    RecordToMoveTo (number)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This is a temporary variable, used to keep status information
        between callbacks. (used with MoveToRecord/MoveToRecordCB)

    replica (object)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This is a replica of a portion of the current recordset.  The
        following hold:
            1.  this.replica[this.FirstRecord] is guaranteed valid
            2.  this.replica[this.LastRecord] is guaranteed valid
            3.  for(i=this.FirstRecord;i<=this.LastRecord;i++)
                    this.replica[i] is guaranteed valid
            4.  this.replica[this.CurrentRecord] is NOT guaranteed valid
                NOTE:
                    if this.FirstRecord<=this.CurrentRecord<=this.LastRecord
                        then it is valid under (3), but don't count on it....

    id (string)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This is what the object was called in the .app file.

    name (string)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This is what the object was called in the .app file.


The osrc has the following JS functions that can be called on itself:

    GiveAllCurrentRecord = osrc_give_all_current_record
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This function is a simple loop through all the children of the osrc,
        calling ObjectAvailable on each one of them, passing the current
        record object as the first parameter to each

    ActionQuery = osrc_action_query
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This takes a query as it's first parameter and a form object as it's
        second.  This was originally intended to be used by external
        obejects (like the form), but now is called internally by
        osrc_action_query_object and osrc_action_order_object.  This handles
        the notification of children that there is a movement in progress
        and the subsequent activating of that query.

    ActionQueryObject = osrc_action_query_object
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This takes a query object as it's first parameter and a form object
        as it's second.  (The form object isn't really required anymore...)
        It builds a query using the passed queryobject and
        this.pendingorderobject and calls ActionQuery with the generated
        query.  Any fancy querying features (>,<,!=, etc.) need to be
        implimented here.

    ActionOrderObject = osrc_action_order_object
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This takes an order object (array of fieldnames and sort order),
        assigns it to pendingorderobject (so it can be found), and calls
        ActionQueryObject with the current queryobject.

    ActionDelete = osrc_action_delete
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This feature is not implimented yet.  There's some code there, but
        it's not right.

    ActionCreate = osrc_action_create
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This feature is not implimented yet.  There's some code there, but
        it's not right.

    ActionModify = osrc_action_modify
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This takes a update/modify object and builds a OSML request and
        sends it, setting osrc_action_modify_cb to be called after
        completion.

    osrc_action_modify_cb
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This function is called when the page called by osrc_action_modify
        is loaded.  It should detect from the page that is returned whether
        it is detected or failed.  However, that is not implimented at this
        point, so we just assume we succeeded.  On success, the replica is
        updated, then OperationComplete is called on the caller with a true
        parameter, and ObjectModified is called on all children.
        On failure, call OperationComplete with a false parameter and don't
        update the replica.

    OpenSession = osrc_open_session
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        If there is a session open already (this.sid defined), directly call
        OpenQuery, otherwise open a new session, and set osrc_open_query to
        be called when it happens.

    OpenQuery = osrc_open_query
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        If there isn't an open session yet (this.sid), then the session id
        is grabbed from the current page.  If there is an active query, it
        is closed, osrc_open_query will be called when that page loads, and
        the query is cleared on the client (this.qid=null).  If there isn't
        an active query, the OSML multiquery request is sent, and the page
        load will be call osrc_get_qid

    osrc_get_qid
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This is called when a page loads that indicates the creation of a
        query.  It reads the query identifier into this.qid, and calls
        DataAvailable on each child.  It then calls ActionFirst on itself.

    CloseQuery = osrc_close_query
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Sets query identifier to null.
        FIXME: Shouldn't this do more???

    CloseObject = osrc_close_object
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Closes the current object.
        FIXME: Is there a current object???

    CloseSession = osrc_close_session
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Makes OSML closesession request.  Sets query id and session id to 
        null.
        FIXME: Shouldn't this do more???

    QueryContinue = osrc_cb_query_continue
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        If nothing is pending, returns 0 -- this is an errant callback.
        Marks the child passed as the first parameter as ready.  Checks if
        all children are ready -- if no, returns 0.
        If this is a query (this.pendingquery set):
            Move all 'pending' stuff (pendingquery, etc.) to their 'real'
            counterparts.  Set all children to not ready (in preparation for
            the next set of callbacks).  Set the TargetRecord,
            CurrentRecord, and OSMLRecord to 0.  Clears the replica.  Set
            LastRecord to 0 and FirstRecord to 1.  Calls OpenSession (which
            will open the query and start getting recordds).
        otherwise it is a movement:
            Call MoveToRecordCB with RecordToMoveTo as a parameter.  Set
            RecordToMoveTo to null.
        Sets pending to false.

    QueryCancel = osrc_cb_query_cancel
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Sets pendingquery to null and pending to false.  Sets all children
        to be not ready.

    RequestObject = osrc_cb_request_object
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Returns 0.
        FIXME: Uh...shouldn't we do something here, or take it out?

    Register = osrc_cb_register
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Adds the object in the first parameter to the children array.

    ActionFirst = osrc_move_first
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Calls MoveToRecord with parameter 1.

    ActionNext = osrc_move_next
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Calls MoveToRecord with parameter CurrentRecord+1.

    ActionPrev = osrc_move_prev
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Calls MoveToRecord with parameter CurrentRecord-1.

    ActionLast = osrc_move_last
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Calls MoveToRecord with parameter Number.MAX_VALUE (since we don't
        know how many records are in the recordset, we try to move to a
        REALLY high numbered record, taking advantage of the fact that the
        OSRC will detect the end of a query and stop, setting CurrentRecord
        back to LastRecord).

    MoveToRecord = osrc_move_to_record
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        This is a helper function for the movement functions. It will return
        with an alert if an attempt is made to move past the beginning.  If
        there is a pending operation, it will return 0 and do nothing.  It
        sets pending, calls IsDiscardReady on all children that need it,
        marking those that don't as ready.  If all children are ready, calls
        this.MoveToRecordCB directly, otherwise will let that happen via
        callbacks.
    
    MoveToRecordCB = osrc_move_to_record_cb
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Called by osrc_cb_query_continue if it detects that the current
        request is a movement (this.pendingquery is not set).  If the
        requested record is in the replica, it is immediately returned.
        If the data is before the replica window, the current query is 
        closed, and a new one opened far enough back to ge the record 
        needed.  If the data is after the current window, the next several
        (this.readahead) records are read in, looking for the requested 
        record.

    ScrollPrev = osrc_scroll_prev
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        If FirstRecord does not equal 1 (at beginning), calls ScrollTo with
        parameter FirstRecord-1.

    ScrollNext = osrc_scroll_next
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Calls ScrollTo with parameter LastRecord+1.

    ScrollPrevPage = osrc_scroll_prev_page
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        If FirstRecord is greater than replicasize, calls ScrollTo with
        parameter FirstRecord-replicasize, otherwise calls ScrollTo with
        parameter 1.

    ScrollNextPage = osrc_scroll_next_page
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Calls ScrollTo with parameter LastRecord+replicasize.

    ScrollTo = osrc_scroll_to
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Called by all the Scroll{Next,Prev}(Page)? functions and directly by
        the dynamic table widget.  Sets TargetRecord to the first parameter.
        If the desired record is in the replica, calls TellAllReplicaMoved,
        sets pending to false, and returns.  Otherwise, if TargetRecord is
        less than FirstRecord:
            sets startat to the more sensible of the following:
                FirstRecord-scrollahead
                1
                TargetRecord
            sets osrc_open_query_startat to be run on next page load.
            if there is a query, closes it (which loads a page which runs
            osrc_query_startat), otherwise, runs it directly.
            Returns 0.
        otherwise (data is farther past the replica):
            If there is an active query, sets osrc_fetch_next to be run on
            next page load, and makes request for next record.  Otherwise
            sets TargetRecord to be LastRecord and calls
            TellAllReplicaMoved.
            Returns 0.

    TellAllReplicaMoved = osrc_tell_all_replica_moved
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Checks for the status of a ReplicaMoved function on all children and
        calls it if present.

    InitQuery = osrc_init_query
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Sets init to true, and calls ActionQueryObject with two null
        parameters.  This will cause the initial query request to be made.

    cleanup = osrc_cleanup
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Returns 0.
        FIXME: This probably needs to have some more stuff added to it.

    osrc_fetch_next
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        If there is no current query (qid==null), abort with an error.
        If there are less than 2 links on the page:
            If this was a move operation, set CurrentRecord equal to 
            LastRecord and call GiveAllCurrentRecord, otherwise call it was
            a scroll operation, call TellAllReplicaMoved.
            Set pending to false and close the query
        Otherwise:
            loop through the links, reading the data into the replica.  If
            the replica size expands beyond the size it should be,
            automatically delete records on the opposite end of the
            expansion, which makes this works like a sliding window.
            If we haven't gotten to the record we're looking for, we send a
            new request and keep on going.  Otherwise ensure we have a full
            replica (making another request to fill it if we don't).  If
            this was a move operation, call GiveAllCurrentRecord, otherwise 
            it was a scroll operation, call TellAllReplicaMoved.

