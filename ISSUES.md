GUI
===
    * Bug: Repeatedly clicking the load and save icons and closing the dialogs with x sometimes makes the program messages like,

          QXcbConnection: XCB error: 3 (BadWindow), sequence: 1352, resource id: 7826577, major code: 40 (TranslateCoords), minor code: 0

    * Bug/Annoyance: Application name is shown behind window title even in simple dialogs (about box, file dialogs, ...)

    * Decision: Use theme icons on linux or just use the same icons everywhere?
      http://mithatkonar.com/wiki/doku.php/qt/icons

    * DoubleView:
        
       !* Why doesn't it show large numbers in scientific notation?
            * Answer: Does only show in scientific when precision is lower than number of digits in "normal" format.
            * New Question: Show in scientific with higher precision?

        * Display large numbers in engineering format (e+03, e+06, e+09, ...) instead of scientific
            * Probably requires making custom conversion methods...

    * SeriesView:
        
        * Implement copy, cut and paste

        * Find solution for default values and validation

       !* Length: negative numbers shouldn't be accepteed

    * ProfileEditor
        
        * Draw the arcs that are) marked in the table in red, so users immediately see what they are editing or deleting

        * NumberGroup below the profile table should not expand vewrtically. Also: Is there a better place for this?

    * Find a solution to localised numbers.
        a) Use the C locale, convert local decimal separator to dot manually
        b) Use the default locale and somehow get rid of the thousands separator

    * Maybe the settings should be in the simulation menu and toolbar.

    * Plot:

        * Enable scrolling and zooming.

        * Add a contect menu action to reset the plot (only relevant if user interaions are enabled).

       !* Handle errors when exporting.

        * Can (and should) the file menus for plot export and the main one "remember" different locations?

        * Allow selecting curves with right click which then also opens the context menu

        * Tick at axis end

        * Make it possible to add a series and define its style in one method call

       !* Change the picking color? Or don't make all the curves blue?

        * Make sure it uses the same locale as the rest of the program. (Currently not the case.)

        * Update to QCustomPlot 2.0 (currently beta)

        * Find out how to align legend with the plot axes and add some margins

    * SettingsDialog:

        * Width of the integer and double views should be the same. Last resort: Set default width of the  integer views.

        * It would be good to have an option to restore the default values

    * Width and height plots: Show the actual arc length if a valid profile curve is defined. (Second x-Axis?)

    * Set the executable icon

   !* MainWindow: Handle exceptions from file operations

    * Add (more) tooltips

    * ShapePlot:
        
       !* Make scrolling speed independent from number of data points

       !* Bug: Aspect ratio initially wrong, corrects itself after changing the parameter or resizing the window.
          (Note: This is not solved, it just doesn't occur anymore since the shape plot is the first tab. Switching tabs and enlarging the window still produce this.)

   * Parameters are not updated when the simulation button is pressed while still editing.
     (Solution: Update document already while typing as long as the input is valid. Fixed for DoubleView, Todo: SeriesView)
     Todo: (DoubleView, IntegerView) Remember entry before editing and revert to that in case of invalid input. That way
     a value that makes sense is used instead of the last digit of some deleted input.

   !* BowEditor: TableView for curvature: Fit complete word "curvature" on label without abbreviating.

    * Try new qcustomplot beta version

   !* BowEditor: Why is the splitter not visible (on linux?)

    * Remember window sizes

FEM
===

    * Find a way to prevent changing the element properties during simulation. Could be done by having the system own
      the elements and only pass a const& of the system in the simulation callbacks.

    * Make simulation callbacks in System templates

    * Make system simulation methods return bool to indicate success instead of throwing exceptions.
      Then throw exceptions with domain-specific error messages in model code.

    * Unify the static solution methods: Implement one method with arbitrary constraint. Load control and displacement control are special cases of this.

    * Does std::function perform dynamic memory allocation? (Used heavily in FEM vector and matrix views)
      https://www.reddit.com/r/cpp_questions/comments/5ga9ar/what_are_downsides_of_using_lambdas/

    * Dynamic cast iterator: Combine with a filter iterator to filter all elements that do not convert?
      (let dynamic_cast return pointer, check for null, then convert to referene)
      This way several element types could be added to one group. Maybe add another key (group + name).

    * Remove/Reduce code duplication from element kinematics

Model
=====

    * InputData: Extend domain such that values in a certain interval can be expressed, e.g. the step factor in ]0, 1]

    * DocItem: value is default constructed and might not be valid until the DocItem is assigned a valid one. Perhaps express this with boost::optional.

    * Make it possible to omit detailed static simulation if only dynamics are needed?

    * Calculate shear stresses and use von Mises stresses in the output data.
      This would give a more realistic stresses for the limb tips (or rather would make the model applicable to the limb tips in the first place)

   !* Handle too short draw length (smaller than brace height)

    * Have a look at https://github.com/nlohmann/json for storing simulation data as MessagePack.

General
=======

    * Restructure. Put code in cpp files, include less stuff in headers to reduce compile times.

    * Decide between using std::array and Eigen::Array, then replace one with the other.

    * Can Eigen::Array<2, dynamic> replace the custom Series class?

   !* tests fail on Windows 7 with MinGW 5.3.0 32bit.

    * Lambdas: Most lambdas could capture only [this], instead of [&]

    * Find a consistent style regarding this->

    * How to avoid using ../../ in includes?

    * Use the GUI editor to rid the setup and layout code, etc... ?

    * Use auto& more often instead of auto?

    * Use Linspace class more often (e.g. for sampling curvces)

    * Use DiscreteLimb class for graphic previews. Give it an optional sampling parameter to use instead of the number of elements.

Platform dependent
==================

    * Look at OS-X Interface guidelines: https://developer.apple.com/library/content/documentation/UserExperience/Conceptual/OSXHIGuidelines/MenuIconsSymbols.html

    * OS-X: Hide menu icons? (Application::setAttribute(Qt::AA_DontShowIconsInMenus, true))

   !* OS-X: How to set the displayed application name to sonething else than the filename?

Ideas for Improvement
=====================

    * Slider in output window could have a go-to menu with options like "maximum limb stress", "arrow separation", "maximum string force", ...

    * Command-line interface:
        * Start simulations, static or dynamic
        * Option to supress Progress? Option to show it in the first place?
        * Create default bow file
        * Option to specify output file
        * Invoke Program on input file: Simulation from command line
        * Invoke Program on output file: Show output window

    * Sampling frequency instead of sampling time?

    * Look at boost book