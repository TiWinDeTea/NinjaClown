use crate::RawApi;
use ninja_clown_bot_sys::{nnj_cell, nnj_cell_kind, nnj_cell_pos, nnj_interaction_kind};
use std::{fmt, iter::Enumerate, ops::Deref};

#[derive(Clone, Debug, Copy, PartialEq)]
#[repr(u32)]
pub enum CellKind {
    Unknown = nnj_cell_kind::CT_UNKNOWN.0,
    Chasm = nnj_cell_kind::CT_CHASM.0,
    Ground = nnj_cell_kind::CT_GROUND.0,
    Wall = nnj_cell_kind::CT_WALL.0,
}

impl CellKind {
    /// # Safety
    /// Calling this function with a value not representable by CellKind
    /// is undefined behavior.
    unsafe fn from_u32_unchecked(v: u32) -> Self {
        std::mem::transmute(v)
    }
}

#[derive(Clone, Debug, Copy, PartialEq)]
#[repr(u32)]
pub enum InteractionKind {
    NoInteraction = nnj_interaction_kind::IK_NO_INTERACTION.0,
    LightManual = nnj_interaction_kind::IK_LIGHT_MANUAL.0,
    HeavyManual = nnj_interaction_kind::IK_HEAVY_MANUAL.0,
    LightMidair = nnj_interaction_kind::IK_LIGHT_MIDAIR.0,
    HeavyMidair = nnj_interaction_kind::IK_HEAVY_MIDAIR.0,
    WalkOnGround = nnj_interaction_kind::IK_WALK_ON_GROUND.0,
}

impl InteractionKind {
    /// # Safety
    /// Calling this function with a value not representable by InteractionKind
    /// is undefined behavior.
    unsafe fn from_u32_unchecked(v: u32) -> Self {
        std::mem::transmute(v)
    }
}

#[derive(Clone, Debug)]
#[repr(transparent)]
pub struct Cell(nnj_cell);

impl Cell {
    pub fn kind(&self) -> CellKind {
        unsafe {
            // # Safety
            // Kind should be a valid CellKind value at any given time
            debug_assert!(self.0.kind.0 <= nnj_cell_kind::CT_WALL.0);
            CellKind::from_u32_unchecked(self.0.kind.0)
        }
    }

    pub fn interaction(&self) -> InteractionKind {
        unsafe {
            // # Safety
            // Kind should be a valid InteractionKind value at any given time
            debug_assert!(self.0.kind.0 <= nnj_interaction_kind::IK_WALK_ON_GROUND.0);
            InteractionKind::from_u32_unchecked(self.0.interaction.0)
        }
    }
}

#[derive(Clone, Debug, Hash, PartialEq, Eq)]
#[repr(transparent)]
pub struct CellPos(nnj_cell_pos);

impl From<nnj_cell_pos> for CellPos {
    fn from(v: nnj_cell_pos) -> Self {
        Self(v)
    }
}

impl From<CellPos> for nnj_cell_pos {
    fn from(v: CellPos) -> Self {
        v.0
    }
}

impl CellPos {
    pub fn new(column: usize, line: usize) -> Self {
        Self(nnj_cell_pos { column, line })
    }

    #[inline]
    pub fn column(&self) -> usize {
        self.0.column
    }

    #[inline]
    pub fn line(&self) -> usize {
        self.0.line
    }

    #[inline]
    pub fn center_x(&self) -> f32 {
        (self.column() as f32) + 0.5
    }

    #[inline]
    pub fn center_y(&self) -> f32 {
        (self.line() as f32) + 0.5
    }
}

#[derive(Clone, Debug)]
pub struct Map {
    grid: Vec<nnj_cell>,
    width: usize,
    height: usize,
    pub(crate) changed: bool, // TODO: expose changed cell positions
}

impl Map {
    pub fn new(raw: &RawApi) -> Self {
        let width;
        let height;

        let grid = unsafe {
            width = (raw.map_width.unwrap())(raw.ninja_descriptor);
            height = (raw.map_height.unwrap())(raw.ninja_descriptor);

            let mut grid = Vec::new();
            grid.resize_with(width * height, nnj_cell::default);
            (raw.map_scan.unwrap())(raw.ninja_descriptor, grid.as_mut_ptr());
            grid
        };

        Self {
            grid,
            width,
            height,
            changed: true,
        }
    }

    pub fn changed(&self) -> bool {
        self.changed
    }

    pub fn cell_at(&self, column: usize, line: usize) -> Option<&Cell> {
        if column >= self.width {
            return None;
        }

        self.grid.get(column + line * self.width).map(|c| {
            unsafe {
                // # Safety
                // Cell struct repr is marked as "transparent", as such &nnj_cell
                // to &Cell cast is okay.
                &*(c as *const nnj_cell as *const Cell)
            }
        })
    }

    pub fn cell_at_pos(&self, pos: &CellPos) -> Option<&Cell> {
        self.cell_at(pos.column(), pos.line())
    }

    #[inline]
    pub fn width(&self) -> usize {
        self.width
    }

    #[inline]
    pub fn height(&self) -> usize {
        self.height
    }

    pub fn iter(&self) -> Iter<'_> {
        Iter(self.grid.iter())
    }

    pub fn iter_pos(&self) -> IterPos<'_> {
        IterPos {
            inner: self.iter().enumerate(),
            width: self.width,
        }
    }

    pub(crate) fn as_mut_ptr(&mut self) -> *mut nnj_cell {
        self.grid.as_mut_ptr()
    }
}

impl fmt::Display for Map {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}x{}", self.width, self.height)?;
        for (i, cell) in self.iter().enumerate() {
            if i % self.width == 0 {
                writeln!(f)?;
            }

            match cell.kind() {
                CellKind::Unknown => write!(f, ".")?,
                CellKind::Chasm => write!(f, "O")?,
                CellKind::Ground => write!(f, " ")?,
                CellKind::Wall => write!(f, "#")?,
            }
        }
        Ok(())
    }
}

impl<'a> IntoIterator for &'a Map {
    type Item = &'a Cell;
    type IntoIter = Iter<'a>;

    fn into_iter(self) -> Self::IntoIter {
        self.iter()
    }
}

pub struct Iter<'a>(core::slice::Iter<'a, nnj_cell>);

impl<'a> Iterator for Iter<'a> {
    type Item = &'a Cell;

    fn next(&mut self) -> Option<Self::Item> {
        self.0.next().map(|c| {
            unsafe {
                // # Safety
                // Cell struct repr is marked as "transparent", as such &nnj_cell
                // to &Cell cast is okay.
                &*(c as *const nnj_cell as *const Cell)
            }
        })
    }
}

pub struct IterPos<'a> {
    inner: Enumerate<Iter<'a>>,
    width: usize,
}

impl<'a> Iterator for IterPos<'a> {
    type Item = CellPosWrapper<'a>;

    fn next(&mut self) -> Option<Self::Item> {
        self.inner.next().map(|(i, c)| {
            let column = i % self.width;
            let line = i / self.width;
            CellPosWrapper {
                inner: c,
                pos: CellPos::new(column, line),
            }
        })
    }
}

pub struct CellPosWrapper<'a> {
    pub inner: &'a Cell,
    pub pos: CellPos,
}

impl<'a> CellPosWrapper<'a> {
    #[inline]
    pub fn column(&self) -> usize {
        self.pos.column()
    }

    #[inline]
    pub fn line(&self) -> usize {
        self.pos.line()
    }
}

impl<'a> Deref for CellPosWrapper<'a> {
    type Target = Cell;

    fn deref(&self) -> &Self::Target {
        self.inner
    }
}
