function [] = spinemlnet_run ()
% start the interface thread
display ('SpineMLNet ML: initialising...');
context = spinemlnetStart (50091);
display ('SpineMLNet ML: initialised.');

% Specify a cleanup function.
cleanupObj = onCleanup(@() spinemlnetCleanup(context));

% First job - add some data for a connection

% My sample experiment requires 3000 points. Here is a sine with
% 3000 points and about 4 3/4 periods:
sine_array = 30 * sin([0:0.01:29.99]);

% Add the sine_array data to spinemlnet for the first connection,
% which is called realtime.
[artn errormsg] = spinemlnetAddData (context, 'realtime', sine_array);
if length(errormsg) > 0
    display (errormsg);
end

% Add the sine_array data to spinemlnet for the second connection,
% which is called realtime2.
[artn errormsg] = spinemlnetAddData (context, 'realtime2', sine_array);
if length(errormsg) > 0
    display (errormsg);
end

% loop until the user presses 'q'
escaped = false;

display ('SpineMLNet ML: Going into loop...');
while escaped == false

    %display ('SpineMLNet ML: Call spinemlnetQuery()');

    % query for current state.
    qrtn = spinemlnetQuery (context);
    % qrtn is:
    % qrtn(1,1): threadFailed
    % qrtn(1,2): updated
    
    % You can also call spinemlnetAddData() within the loop, as
    % often as you like to add to the data which will be passed to
    % the SpineML experiment/simulation/execution.

    pause (1);
    
    if qrtn(1,1) == 1
        % The thread failed, so set escaped to true.
        display ('SpineMLNet ML: The TCP/IP I/O thread seems to have failed. Finishing.');
        escaped = true;
    end

end

display ('SpineMLNet ML: Script finished');
end

function spinemlnetCleanup (context)
    disp('SpineMLNet ML: spinemlnetCleanup: Calling spinemlnetStop');
    spinemlnetStop(context);
    disp('SpineMLNet ML: cleanup complete.');
end
